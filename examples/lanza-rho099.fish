#!/usr/bin/fish

set processors (cat /proc/cpuinfo |grep processor|wc -l)
set processes $processors

set a 1
while test $a -le $processes
    mkdir $a
    cd $a
    ../../ExperimentoCV-E2.tcl 099 $a $processes &
    cd ..
    set a (math $a+1)
end
