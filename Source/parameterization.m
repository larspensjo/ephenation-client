# This file contains Octave definitions of the atmosphere parameterization functions
#
1; # Force Octave to interpret the file as a script, not as a single function

function ret = SunAngleParameterization(cs)
	tmpsunconstant = tan(1.26 * 1.1)
	ret = 0.5 * (atan(max(cs, -0.1975) * tmpsunconstant) / 1.1 + (1-0.26))
endfunction

function ret = SunAngleParameterizationInverse(us)
	tmpsunconstant = tan(1.26 * 1.1);
	ret = tan((2*us-0.74)*1.1)/tmpsunconstant;
endfunction

