#! /bin/sh
cd ../..
sbt "runMain scheduler.scheduler"

cd verilator-tests/scheduler

cp ../../scheduler.v .

echo '/* verilator lint_off UNUSED */' | cat - scheduler.v > temp && mv temp scheduler.v
echo '/* verilator lint_off DECLFILENAME */' | cat - scheduler.v > temp && mv temp scheduler.v


verilator -Wall --trace -cc scheduler.v

cd obj_dir

make -f Vscheduler.mk

cd ..

g++ -I /usr/share/verilator/include -I obj_dir /usr/share/verilator/include/verilated.cpp /usr/share/verilator/include/verilated_vcd_c.cpp instrIssuetest.cpp obj_dir/VinstrIssue__ALL.a -o instrIssue

./instrIssue

gtkwave instrIssue_trace.vcd
