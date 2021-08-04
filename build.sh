clear

# Clean build always lol
echo "**** Cleaning ****"
rm -r out
mkdir out
make clean && make distclean && make mrproper

# auto clang cloner/updater
echo "**** Cleaning ****"
if [ -r clang ]; then
	echo "old clang found! would u like to clone again latest clang? ( Y/N )"

	read answer
	if [[ $answer == 'Y' ]]; then
		echo "OK, cleaning old clang files"
		rm -rf clang
		git clone https://github.com/kdrag0n/proton-clang.git --depth=1 clang
	elif [[ $answer == 'y' ]]; then
		echo "OK, cleaning old clang files"
		rm -rf clang
		git clone https://github.com/kdrag0n/proton-clang.git --depth=1 clang
	elif [[ $answer == 'N' ]]; then
		echo "OK, continuing with compilation"
	elif [[ $answer == 'n' ]]; then
		echo "OK, continuing compilation"
	fi

else
  echo "clang not found!, git cloning it now...."
  git clone https://github.com/kdrag0n/proton-clang.git --depth=1 clang

fi


KERNEL_DEFCONFIG=RMX1921_defconfig
Device="Realme XT (RMX1921)"
ANYKERNEL3_DIR=$PWD/AnyKernel3/
KERNELDIR=$PWD/
export PATH="${PWD}/clang/bin:${PATH}"
export ARCH=arm64
export SUBARCH=arm32
export KBUILD_COMPILER_STRING="$(${PWD}/clang/bin/clang --version | head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//')"
# Speed up build process
MAKE="./makeparallel"

BUILD_START=$(date +"%s")
blue='\033[0;34m'
cyan='\033[0;36m'
yellow='\033[0;33m'
red='\033[0;31m'
green='\033[0;32m'
nocol='\033[0m'

echo ""
echo -e "$blue***********************************************"
echo "**** Kernel defconfig is set to $KERNEL_DEFCONFIG ****"
echo -e "***********************************************$nocol"
echo "--------------------------------------------------------"
echo -e "$blue***********************************************"
echo "            BUILDING KERNEL          "
echo -e "***********************************************$nocol"
echo ""
make $KERNEL_DEFCONFIG O=out
make -j$(nproc --all) O=out \
                      ARCH=arm64 \
                      CC=clang \
                      CROSS_COMPILE=aarch64-linux-gnu- \
                      CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
                      NM=llvm-nm \
                      OBJCOPY=llvm-objcopy \
                      OBJDUMP=llvm-objdump \
                      STRIP=llvm-strip

echo "**** Verify Image.gz-dtb ****"
if [ -f out/arch/arm64/boot/Image.gz-dtb ]; then
  echo "**** Removing leftovers ****"
  cd $ANYKERNEL3_DIR/
  make clean
  cd ..

  echo "**** Copying Image.gz-dtb ****"
  cp $PWD/out/arch/arm64/boot/Image.gz-dtb $ANYKERNEL3_DIR/
  cp $PWD/out/arch/arm64/boot/dtbo.img $ANYKERNEL3_DIR/

  echo "**** Time to zip up! ****"
  cd $ANYKERNEL3_DIR/
  make zip
  cd ..
  echo "**** Done, here is your build info ****"

  BUILD_END=$(date +"%s")
  DIFF=$(($BUILD_END - $BUILD_START))
  echo -e "$yellow Device:-$Device.$nocol"
  echo -e "$yellow Build Time:- Completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds.$nocol"

else
  echo "**** Build Failed ****"
  echo "**** Removing leftovers ****"
  rm -rf out/
  BUILD_END=$(date +"%s")
  DIFF=$(($BUILD_END - $BUILD_START))
  echo -e "$red mission failed we'll get em next time, Build failed in:- $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds.$nocol"
fi