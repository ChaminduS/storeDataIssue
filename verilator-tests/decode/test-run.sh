#! /bin/sh
cd ../..
sbt "runMain decode.DecodeUnit"

cd verilator-tests/decode

mv ../../decode.v .

echo '/* verilator lint_off UNUSED */' | cat - decode.v > temp && mv temp decode.v
echo '/* verilator lint_off DECLFILENAME */' | cat - decode.v > temp && mv temp decode.v


verilator -Wall --trace -cc decode.v

cd obj_dir

make -f Vdecode.mk

cd ..

g++ -I /usr/share/verilator/include -I obj_dir /usr/share/verilator/include/verilated.cpp /usr/share/verilator/include/verilated_vcd_c.cpp decodetest.cpp obj_dir/Vdecode__ALL.a -o decode

./decode
