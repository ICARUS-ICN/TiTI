#!/usr/bin/fish

for a in 1 2 3 4 5 6 7 8 9 10 11 12 13 14
    mkdir $a
    cd $a
    ../../ExperimentoCV-rho-098.tcl 098 $a 14 &
    cd ..
end
