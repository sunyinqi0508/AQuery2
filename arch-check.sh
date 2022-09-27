ARCH=`uname -m`
ARCH2=`arch`
echo "Current architechure: (uname -m) => $ARCH; (arch) => $ARCH2"
echo Current shell: $SHELL
PASSED=1
for i in python3 c++ make ranlib libtool $SHELL $CXX
do
	FILEPATH=`which $i`
	FILEINFO=`file $FILEPATH`
	if [[ $FILEINFO =~ $ARCH ]]; then
		echo $i@$FILEPATH: passed
	else
		echo "\033[1;31mERROR\033[0m: Architecture of $i is not $ARCH: $FILEINFO"
		PASSED=0
	fi
done

if [[ PASSED -eq 1 ]]; then
	echo "\033[1;32mBinary archtechure check passed\033[0m"
fi

