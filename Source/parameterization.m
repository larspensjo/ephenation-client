# This file contains Octave definitions of the atmosphere parameterization functions
#
1; # Force Octave to interpret the file as a script, not as a single function

disp "SunAngleParameterization(cs)"
function ret = SunAngleParameterization(cs)
	tmpsunconstant = tan(1.26 * 1.1)
	ret = 0.5 * (atan(max(cs, -0.1975) * tmpsunconstant) / 1.1 + (1-0.26))
endfunction

disp "SunAngleParameterizationInverse(us)"
function ret = SunAngleParameterizationInverse(us)
	tmpsunconstant = tan(1.26 * 1.1);
	ret = tan((2*us-0.74)*1.1)/tmpsunconstant;
endfunction

disp "ViewAngleParameterized(cv,h)"
function ret = ViewAngleParameterized(cv, h)
	R_Earth = 6371*1000;
	ch = - sqrt(h * (2 * R_Earth + h)) / (R_Earth + h);
	if (cv > ch)
		ret = 0.5 * power((cv-ch) / (1-ch), 0.2) + 0.5;
	else
		ret = 0.5 - 0.5 * power((ch-cv) / (ch+1), 0.2);
	endif
endfunction

disp "ViewAngleParameterizedInverse(uv,h)"
function ret = ViewAngleParameterizedInverse(uv, h)
	R_Earth = 6371*1000;
	ch = - sqrt(h * (2 * R_Earth + h)) / (R_Earth + h);
	if (uv > 0.5)
		ret = ch + power((uv-0.5)*2, 5) * (1 - ch);
	else
		ret = ch - power(1 - uv*2, 5) * (1 + ch);
	endif
endfunction

disp "DensityRayleigh(uv,h)"
function ret = DensityRayleigh(h)
	ret = exp(-h / 8000);
endfunction

disp "getStepSize"
function ret = getStepSize(h)
	stepAtm = 4000;
	stepGround = 100;
	k = log(stepAtm / stepGround) / 80000;
	ret = stepGround * exp(k * h);
endfunction
