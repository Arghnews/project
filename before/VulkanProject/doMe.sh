#!/bin/bash

main() {

	executable="exec"

	for arg in "$@"; do
		case $arg in
			clean|-clean|--clean)
				# args to this passed as literals - important as used in globbing
				ifExistsRemoveNoDoubleQuotes '*.d' # remove dependency files too
				ifExistsRemoveNoDoubleQuotes '*.o' # remove object files to force all recompile
				ifExistsRemoveNoDoubleQuotes '*.spv'  # remove compiled shaders
				ifExistsRemoveNoDoubleQuotes "$executable"
				exit 0
				;;
			"*")
				# nothing
				;;
		esac
	done

	# currently only allows dependency of x.cpp on x.o
	#"MyCamera"
	#"MyVulkan"
	#"main"
	#"MyInputClass"
	cppFiles=( 
	"Camera"
	"Thing"
	"World"
	"MyVulkan"
	"Helper"
	"Physics"
	"TheMaster"
	"Input"
	)
	shaders=0

	changed=0

	exitCodeSum=0

	# vert
	shaderVert="shaders/shader.vert"
	shaderVertObj="vert.spv"

	# frag
	shaderFrag="shaders/shader.frag"
	shaderFragObj="frag.spv"

	compiler="g++"

	compilerFlags="-O3 -Wall -std=c++11 -MMD"

	libraries="-L../VulkanSDK/1.0.30.0/x86_64/lib/ -lvulkan -L../glfw-3.2.1/lib/ -lglfw"

	includes="-I../glfw-3.2.1/include/ -I../VulkanSDK/1.0.30.0/x86_64/include -I../glm -I../stb -I../tinyobjloader"

	compileShader="$VULKAN_HOME/x86_64/bin/glslangValidator -V"

	shaderFiles=(
	"shader.vert"
	"shader.frag"
	)

	# converts dir/ or dir -> dir/, because even the guy who wrote this
	# won't be able to remember whether to feed it dir/ or dir
	shaderDir="shaders"
	shaderDir=$(echo $shaderDir | tr -s /)
	shaderDir=$shaderDir"/"

	i=0
	n=${#shaderFiles[@]}

	while [ $shaders -eq 0 ] && [ $i -lt $n ]; do # appends the shader directory onto it
		shader=${shaderFiles[i]} # shader file name
		shader=$shaderDir$shader # append path to shaders on front
		x=$(echo "$shader" | sed -E "s/.*\.(.*)/\1/")
		compiledShader=$x'.spv' # frag.spv
		x=$(newerThan "$shader" "$compiledShader")
		if [ $x -ne 0 ]; then # then need recompile
			$compileShader $shader
			changed
		fi
		let i=$i+1
		if [ $i -gt 20000 ]; then
			break
		fi
	done

	fileList=""
	dotO=".o" 
	dotCpp=".cpp"
	dotHpp=".hpp" # crude but works for x.cpp and x.hpp -> x.o
	for f in "${cppFiles[@]}"; do
		# x is 0 if needs a change, 1 if changed
		# header only if exists
		theCpp=$(newerThan "$f$dotCpp" "$f$dotO")
		theHeader=0
		for header in $(grep -E "^#include \"(.*\.hpp)\"" "$f$dotCpp" | sed "s/^#include [\"|']//g" | sed "s/.$//g"); do
			#echo For file "$f$dotCpp" found header "$header"
			let theHeader=$theHeader+$(newerThan "$header" "$f$dotO")
		done;
		if [ "$theCpp" -ne 0 ]; then
			createObjFile "$f$dotCpp"
			changed
		elif [ "$theHeader" -ne 0 ]; then
			createObjFile "$f$dotCpp"
			changed
		fi
		fileList="$fileList$f$dotO "
	done
	# if any files didn't compile correctly etc exit
	if [ $exitCodeSum -ne 0 ]; then
		exit 1
	elif [ $changed -ne 0 ] || [ ! -f $executable ]; then
		# link
		run link "$fileList"
	fi

	if [ $exitCodeSum -eq 0 ]; then
		run="./$executable"
		run $run
	fi
}
function headersIn() {
	if [ -z ${1+x} ]; then
		echo "Provide file to check for dependencies"
		return 1
	fi
	echo $1 depends on
	for f in "${dependsOn[@]}"; do
		echo $f
	done
}

# used to know if should relink files - if nothing changed then don't bother
# unless executable doesn't exist
function changed() {
	let changed=$changed+1
}

# functions passed to this should not be quoted, their args is up to you
# be careful with the quotes!
function run() {
	"$@" # must be quoted
	exitCode=$?
	let exitCodeSum=$exitCodeSum+$exitCode
	return $exitCode
}

# $1 should be file type, can be glob wildcard
# crucial no quotes, don't want to expand *.o -> x.o y.o etc
function ifExistsRemoveNoDoubleQuotes() {
	if [ -z ${1+x} ]; then
		echo "Function to remove files must be called with what file/files to clean"
		exit 1
	fi
	ls $1 &>/dev/null # all output to dev null
	any=$?
	[ $any -eq 0 ] && rm $1
}

function newerThan() {
	local f1="$1"
	local f2="$2"
	if [ "$f1" -nt "$f2" ]; then
		echo 1
	else
		echo 0
	fi
}

function createObjFile() { # arg should be file to create object file
	if [ -z ${1+x} ]; then
		echo "Provide cpp file"
		return 1
	fi
	local x="$compiler -c $compilerFlags $includes $1"
	echo Recompiling $1 ...
	run $x
}

function link() {
	if [ -z ${1+x} ]; then
		echo "Provide cppFiles to link"
		return 1
	fi
	local __cppFiles=$1
	local x="$compiler -o $executable $compilerFlags $includes $__cppFiles $libraries"
	echo $x
	run $x
}

main "$@"
