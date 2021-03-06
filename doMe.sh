#!/bin/bash

main() {

    # Does not work currently for files with space in the name
    # To be fair if you have files with spaces in the name
    # on linux you should probably be shot

    # Also not fully sure how to make it work with different file types
    # yet, ie. Vulkan spv compiled shaders or c files that need gcc etc

    # Also situations where files aren't in same folder as make etc.
    # Everything assumed in the working dir atm

    # "Edge" ish case - script currently doesn't check executable -> all cpp files

    # object dir per executable

    # when compiling lots of files and one errors, silently continue
    # recompiling all the files that worked

    # for now if run ./doMe.sh --client --clean will remove all objects

    export CPATH=$CPATH:include
    export LIBRARY_PATH=$LIBRARY_PATH:lib
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:lib

	executable="game" # .exec will be appeneded

    game=(
    "Physics.cpp"
    "Shader.cpp"
    "G_Cuboid.cpp"
    "L_Cuboid.cpp"
    "P_State.cpp"
    "Actor.cpp"
    "Window_Inputs.cpp"
    "Main.cpp"
    "AABB.cpp"
    "Octree.cpp"
    "Actors.cpp"
    "World.cpp"
    "Sender.cpp"
    "Receiver.cpp"
    "Packet_Header.cpp"
    "Packet_Payload.cpp"
    "Packet.cpp"
    "Connection.cpp"
    )

    client=(
    "Sender.cpp"
    )

    server=(
    "Receiver.cpp"
    )

    network=(
    "Network_Test.cpp"
    "Receiver.cpp"
    "Sender.cpp"
    "P_State.cpp"
    "Packet_Header.cpp"
    "Packet_Payload.cpp"
    "Packet.cpp"
    "Connection.cpp"
    )

    Test=(
    "Test.cpp"
    )

    cppFiles=("${game[@]}")

	compiler="g++"

	compilerFlags="-O3 -std=c++11 -Wno-narrowing"

    #libraries="-lGLEW -lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi"
#g++ main.o -o main.exec -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi
    # at least on my home ubuntu, glfw -> dynamic/shared, glfw3 static
    # lz for compression - zlib
    libraries="-lGLEW -lGL -lGLU -lglfw -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lz -lfreetype"
    # older version of glm
    # glew, glfw libs (dynamic) in your LIBRARY_PATH/LD_LIBRARY_PATH
    # glew, glfw, glm headers in your CPATH
    #export CPATH=$CPATH:$HOME/include
    #export LIBRARY_PATH=$LIBRARY_PATH:$HOME/lib
    #export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/lib

    ubuntu_include_dir="/usr/local/include/freetype2"

    dcs_include_dir="/dcs/14/u1419657/include/freetype2"

    who="$(whoami)"

    if [ "$who" == "u1419657" ]; then
        chosen_dir="$dcs_include_dir"
    else
        chosen_dir="$ubuntu_include_dir"
    fi

	includes="-Icereal/include -Iasio/include -I$chosen_dir"

    #verbose=true # if "true" will print
    verbose=false
    silent=false # if "true" will print

    MMD="-MM" # -M default, MM excludes sys headers
    genDependencies="$compiler $compilerFlags $includes $MMD"

    objectDir="objects"

    # end of user parameters

    args=()

    # there is a case where if you need to pass say --verbose to program and/or to script
    # currently only script will get it, if this case ever comes to pass/I can be bothered
    # shall add a pass through $@ to find if -- is present, if so then all args before --
    # passed to script, args after passed to program

    # special case for verbose, eg. want to run ./script clean -v won't work without this

	for arg in "$@"; do
		case $arg in
            -v|--v|-verbose|--verbose)
                verbose=true
                v "Verbose output enabled"
                ;;
		esac
    done

    # if want to only check if these files needs recompile
    # to short circuit dcs's slow as crap file new/old lookup tests
    onlyRecompileThese=(
    )

	for arg in "$@"; do
		case $arg in
			clean|-clean|--clean)
				# args to this passed as literals - important as used in globbing
				remove "$objectDir"/'*.o' # remove object files to force all recompile
				remove "$executable"
				exit 0
				;;
            -v|--v|-verbose|--verbose)
                ;;
            -q|--q|-quick|--quick)
                compilerFlags=$(echo "$compilerFlags" | sed -E "s/( -O[0-3])|(-O[0-3] )//g")
                v "Removing compiler optimisation options if any"
                ;;
            --client)
                cppFiles=("${client[@]}")
                executable="client"
                ;;
            --server)
                cppFiles=("${server[@]}")
                executable="server"
                ;;
            --network)
                cppFiles=("${network[@]}")
                executable="network"
                ;;
            --test)
                cppFiles=("${Test[@]}")
                executable="Test"
                ;;
            *.c|*.cc|*.cpp)
                v "Argument recognised as src file as $arg, will check recompile on only this and other src files"
                onlyRecompileThese+=("$arg")
                ;;
			*)
                args+=("$arg")
                # add args passed to script to arg array
                # these args will be passed to the program
				;;
		esac
	done

    executable="$executable"'.exec'

    # if want to remove args etc should do in loop above or here
    # argsStr is for printout etc

    argsStr="${args[@]}"

    v "${#args[@]} argument(s) to be passed to program: $argsStr"

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
    local compileFiles="${cppFiles[@]}"
    if [ "${#onlyRecompileThese[@]}" -gt 0 ]; then
        compileFiles="${onlyRecompileThese[@]}"
    fi
    #for f in ${cppFiles[@]}; do
    for f in ${compileFiles[@]}; do
        v "Generating dependencies for $f using $genDependencies"
        local dependencies="$($genDependencies $f)"
        v "Generated dependencies"
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
        local numberObjects=$(ls 2>/dev/null -Ub1 | grep "^.*\.o$" | wc -l)
        if [ "$numberObjects" -ne ${#recompiles[@]} ]; then
            echoErr "Warning: Moving object files from working dir to $objectDir, but found $numberObjects to move but ${#recompiles[@]} to recompile."
            echoErr "May be moving some objects you don't want to be moved (will add confirm yes/no in future)"
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
            #local objectFile="$(echo $f | sed -E "s/^(.*)\.c(pp)?$/\1.o/g")"
            local objectFile="$(echo $f | sed -E "s/^(.*)\.c(pp|c|)$/\1.o/g")"
            if [ "$objectFile" == "$f" ]; then
                # ie. bla.cp -> bla.cp, could not objectify ie. it has a dumb name
                echoErr "Could not create a new object file name from $f, it should be of form bla.(c|cc|cpp)"
            fi
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

    exe="./$executable"

    local argsLen=${#args[@]}
    local runPrint=""
    local execPrint="Running $exe"
    local argsPrint="with args $argsStr"

    runPrintout="$runPrintout$execPrint"
    if [ "$argsLen" -gt 0 ]; then
        runPrintout="$runPrintout $argsPrint"
    fi
    p "$runPrintout"
    $exe $argsStr

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
            obj=$(objectify "$obj")
            let i=$i+1 # only want to find object on first line
        elif [ "$item" == \\ ]; then
            :
        else
            # quick fix to make this shit faster on NFS like DCS
            has=$(echo "$item" | grep -Eq "(asio|glm)" && echo 1)
            if [ "$has" == "1" ]; then
                :
            else
                dependencies+=("$item")
            fi
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
function remove() {
	if [ -z ${1+x} ]; then
		echo "Function to remove files must be called with what file/files to clean"
		exit 1
	fi
	ls $1 &>/dev/null # all output to dev null
	local any=$?
    local rmRet=0
    if [ "$any" -eq 0 ]; then
        v "Deleting $1"
        rm $1
        rmRet=$?
        let ret=$any+$rmRet
        return $ret
    else
        v "No files to delete matching $1"
        return 0
    fi
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
