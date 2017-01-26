#!/bin/bash

main() {

    # Does not work currently for files with space in the name
    # To be fair if you have files with spaces in the name
    # on linux you should probably be shot

    # Also not fully sure how to make it work with different file types
    # yet, ie. Vulkan spv compiled shaders or c files that need gcc etc

    # Also situations where files aren't in same folder as make etc.
    # Everything assumed in the working dir atm

	executable="exec"

	cppFiles=( 
    "Main.cpp"
    "Window_Inputs.cpp"
    "Camera.cpp"
    "Shader.cpp"
	)

	compiler="g++"

	compilerFlags="-O3 -std=c++11"

    #libraries="-lGLEW -lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi"
#g++ main.o -o main.exec -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi
    # at least on my home ubuntu, glfw -> dynamic/shared, glfw3 static
    libraries="-lGLEW -lGL -lGLU -lglfw -lX11 -lXxf86vm -lXrandr -lpthread -lXi"

	includes=""

    #verbose=true # if "true" will print
    verbose=false
    silent=false # if "true" will print

    MMD="-M" # -M default, MM excludes sys headers
    genDependencies="$compiler $compilerFlags $MMD"

    objectDir="objects"

    # end of user parameters

	for arg in "$@"; do
		case $arg in
			clean|-clean|--clean)
				# args to this passed as literals - important as used in globbing
				removeNoDoubleQuotes "$objectDir"/'*.o' # remove object files to force all recompile
				removeNoDoubleQuotes  '$executable'
				exit 0
				;;
			"*")
				# nothing
				;;
		esac
	done

    # changes /usr/lib//bla/blabla/// -> /usr/lib/bla/blabla/
    objectDir=$(echo "$objectDir" | sed -E "s:/{1,}:/:g")
    # since this won't work if you're placing your objects in the root directory
    # (like some kind of idiot...)
    [ "$objectDir" == '/' ] && echoErr "Cannot use root directory as object directory" && exit 1
    # changes /usr/lib/bla/ -> /usr/lib/bla
    objectDir=${objectDir%/}
    v "Object directory is $objectDir"
    if [ ! -d "$objectDir" ]; then
        # can fail, ie. file called "$objectDir" exists
        p "Object directory does not exist, creating object directory called $objectDir"
        mkdir "$objectDir"
        [ $? -ne 0 ] && echoErr "Could not create object directory $objectDir" && exit 1
    fi

    for f in ${cppFiles[@]}; do
        if [ ! -f "$f" ]; then
            echoErr "Src file $f cannot be found" && exit 1
        fi
    done

    local recompiles=()

    # Build dependency lists for all cpp files in cppFiles
    # And add them to recompiles list
    for f in ${cppFiles[@]}; do
        v "Generating dependencies for $f using $genDependencies"
        local dependencies="$($genDependencies $f)"
        needsCompile "$dependencies"
        local ret=$?
        if [ $ret -ne 0 ]; then # needs recompile
            recompiles+=("$f")
        fi
    done

    # build objects that need recompile
    # don't put quotes around this expansion
    if [ ${#recompiles[@]} -gt 0 ]; then
        local compilelist=""

        for f in ${recompiles[@]}; do
            # append everything that needs recompilng to compile list
            compilelist="$compilelist $f"
        done

        local compile="$compiler -c $includes $compilerFlags"
        local compileRun="$compile $compilelist"
        p "Compiling ->" $compileRun
        $compileRun
        # if any files didn't compile correctly etc exit
        [ $? -ne 0 ] && echoErr "Compile failed" && exit 1
        v "Compiled successfully"

        # move objects from current dir to objects dir
        # alternatively could run gcc compile -c -o for every cpp/object
        # number of object files in current directory
        # -U unordered, b escapes newlines, 1 -> 1 entry per line, counts objects in current working dir
        local numberObjects=$(ls 2>/dev/null -Ub1 | grep ^.*\.o$ | wc -l)
        if [ "$numberObjects" -ne ${#recompiles[@]} ]; then
            echoErr "Warning: Moving object files from working dir to object dirs, but found $numberObjects to move but ${#recompiles[@]} to recompile.\nMay be moving some objects you don't want to be moved."
        fi
        v "Moving *.o to $objectDir/"
        mv *.o $objectDir/
    fi

    # if any files were recompiled or if the executable doesn't exist
    # link the objects
	if [ ${#recompiles[@]} -gt 0 ] || [ ! -f $executable ]; then
        local link="$compiler $compilerFlags $includes"
        local objectlist=""
        for f in ${cppFiles[@]}; do
            local objectFile="$(echo $f | sed -E "s/^(.*)\.c(pp)?$/\1.o/g")"
            local obj=$(objectify "$objectFile")
            objectlist="$objectlist $obj"
        done
        local after="$libraries -o $executable"
        local linkCmd="$link $objectlist $after"
        p "Linking ->" $linkCmd
		$linkCmd # link
        # if linker errored
        [ $? -ne 0 ] && echoErr "Linking failed" && exit 1
        v "Linked successfully"
	fi

    run="./$executable"
    p "Running $run"
    ./"$run"

    exit $?
}

function objectify() {
	if [ -z ${1+x} ]; then
		#echo "Func to change cpp -> object files should have cpp file as first arg"
        echo "Currently just lazy, changes A.o -> objects/A.o"
		exit 1
    fi
    local o="$objectDir/$1"
    echo "$o"
    return 0
}

# takes dependency list as generated by say gcc -M A.cpp
# returns 1 if the object A.o in that list needs a recompile, else 0
function needsCompile() {
	if [ -z ${1+x} ]; then
		echo "Function to parse dependencies should have var with them in as first arg"
		exit 1
    fi
    local deps=$1
    local obj=""
    local dependencies=()
    # obj -> object deps that needs recompile or not
    # obj -> A.o
    # fill dependencies with dependencies of the object file
    local i=0
    for item in ${deps[@]}; do
        if [ $i -eq 0 ]; then
            obj=$(echo "$item" | sed -E "s/^(.*):.*$/\1/g")
            # turns A.c / A.cpp -> A.o
            obj=$(objectify "$obj")
            let i=$i+1 # only want to find object on first line
        elif [ "$item" == \\ ]; then
            :
        else
            dependencies+=("$item")
        fi
    done
    v "${#dependencies[@]} dependencies found for $obj"
    
    # if the obj is older than any dependency, return 1
    for ele in ${dependencies[@]}; do
        #echo "Comparing $obj to $ele"
        if [ "$obj" -nt "$ele" ]; then
            : # obj newer than element
        else
            v "$obj needs recompile, it is newer than $ele"
            return 1
        fi
    done
    v "$obj does not need recompile"

    return 0
}

# $1 should be file type, can be glob wildcard
# crucial no quotes, don't want to expand *.o -> x.o y.o etc
function removeNoDoubleQuotes () {
	if [ -z ${1+x} ]; then
		echo "Function to remove files must be called with what file/files to clean"
		exit 1
	fi
	ls $1 &>/dev/null # all output to dev null
	local any=$?
    v "Deleting $1"
	[ $any -eq 0 ] && rm $1
    return $any
}

function v() { # verbose print
    if [ ! "$silent" == "true" ] && [ "$verbose" == "true" ]; then
        echo "$@"
    fi
}

function echoErr() { # error print
    if [ ! "$silent" == "true" ]; then
        echo "$@" 1>&2
    fi
}

function p() { # print
    if [ ! "$silent" == "true" ]; then
        echo "$@"
    fi
}

main "$@"
