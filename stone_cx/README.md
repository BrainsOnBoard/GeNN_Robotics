# Stone et al. Model of path integration
Published model of Bee path integration, re-implemented using sigmoid units in GeNN. Not configured for GPU simulation so build using the following steps:
* ``genn-buildmodel.sh -c model.cc`` to build CPU-only model
* ``make -f Makefile`` to build simulated version, ``make -f MakefileVicon`` to build robot version using Vicon tracking and ``make -f MakefileRobot`` to build robot version using optical flow for velocity and magnetic sensor for heading.

Outward path for both robot models is controlled using joystick device. Pressing 1st button (A on Xbox360 controller) starts homing and 2nd button (B on Xbox360 controller) stops model.

Stone, T., Webb, B., Adden, A., Weddig, N. B., Honkanen, A., Templin, R., … Heinze, S. (2017). An anatomically constrained model for path integration in the bee brain. Current Biology, (in press), 1–17. https://doi.org/10.1016/j.cub.2017.08.052