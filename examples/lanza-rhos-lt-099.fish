#!/usr/bin/fish

set processors (cat /proc/cpuinfo |grep processor|wc -l)
set processes $processors

set a 1
while test $a -le $processes
    mkdir $a
    cd $a
    ../../ExperimentoCV-D.tcl 08 $a $processes &
    cd ..
    set a (math $a+1)
end

set a 1
while test $a -le $processes
    while test (jobs|wc -l) -ge $processors
	sleep 1
    end
    cd $a
    ../../ExperimentoCV-D.tcl 09 $a $processes &
    cd ..
    set a (math $a+1)
end

set a 1
while test $a -le $processes
    while test (jobs|wc -l) -ge $processors
	sleep 1
    end
    cd $a
    ../../ExperimentoCV-D.tcl 095 $a $processes &
    cd ..
    set a (math $a+1)
end

set a 1
while test $a -le $processes
    while test (jobs|wc -l) -ge $processors
	sleep 1
    end
    cd $a
    ../../ExperimentoCV-D.tcl 098 $a $processes &
    cd ..
    set a (math $a+1)
end
