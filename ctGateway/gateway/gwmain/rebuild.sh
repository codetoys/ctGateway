
#

cd ../dlt645
cp makefile.mk makefile
rm *.o ../../lib/libdlt645.a
make all

cd ../gwBuiltIn

rm *.o ../../lib/libgwbuiltin.a
make all

cd ../gwprotocol

rm *.o ../../lib/libprotocol_demo.*
make all

cd ../gwmain

cp makefile.mk makefile
rm *.o ../../lib/libgwmain.* ../bin/gwmain.exe ../publish/gwmain.exe
../../platform/_makecpp.sh
make all

