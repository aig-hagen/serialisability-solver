#!/bin/bash
./serial-solver -p DS-PR -fo tgf -f examples/A-1-ferry2.pfile-L2-C1-05.pddl.3.cnf.tgf -a a138
./serial-solver -p DS-PR -fo tgf -f examples/A-1-BA_40_80_5.tgf -a a36
./serial-solver -p DS-PR -fo tgf -f examples/B-2-comune-di-castellanza_20151218_2338.gml.80.tgf -a a31
./serial-solver -p DS-PR -fo tgf -f examples/A-1-BA_60_70_3.tgf -a a12
./serial-solver.sh -p DS-PR -fo tgf -f examples/T-1-cascadespoint-or-us.gml.20.tgf -a a5