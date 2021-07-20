#!/usr/bin/fish

set processors (cat /proc/cpuinfo |grep processor|wc -l)
set processes $processors

for rho in 09 095 098 099
#for rho in 08
        for tipo in ExperimentoCV-DM1.tcl ExperimentoCV-MD1.tcl ExperimentoCV-MM1.tcl
	set a 1
	while test $a -le $processes
	    if set -q prev_tipo
		while test (jobs|wc -l) -ge $processors
		    wait -n
		end
	    else
		mkdir $a
	    end
	    cd $a
	    ../../$tipo $rho $a $processes &
	    cd ..
	    set a (math $a+1)
	end
	set prev_tipo $tipo
    end
end
