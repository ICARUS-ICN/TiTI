TiTI
======

Time to Idle Simulator of pooling service systems

[![build](https://github.com/ICARUS-ICN/TiTI/actions/workflows/main.yml/badge.svg)](https://github.com/ICARUS-ICN/TiTI/actions/workflows/main.yml) 
[![DOI](https://zenodo.org/badge/386551946.svg)](https://zenodo.org/doi/10.5281/zenodo.10912537)

## Overview

This tool simulates an pooling service discipline such as FDDI and it can use as Control Variates the Time to Idle process.

## USAGE:
	unixfddi <configuration file> [OPTIONS|FLAGS]

### FLAGS: 
gg1  => simulates a simple G/G/1 queue
fast => simulates with unlimited queueing capacity
anal_est => estimates queue waiting times confidence intervals

### OPTIONS: s<seed>q<quality>t<relative tolerance>T<transitory interval>n<reserved circuits>m<minimum sample size>aAbf]
seed <seeed>
clock <transient period>

## Legal
Copyright ⓒ 199x–2021 Andrés Suárez González <asuarez@det.uvigo.es>.

This simulator is licensed under the BSD 2-Clause License. For information see LICENSE.

