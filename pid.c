#include "pid.h"


void initPid ( pid_state_t * state ) {
	state -> error_integrated = 0.0;
	state -> error_previous = 0.0;
}

double pidUpdate ( pid_state_t *state,
					double error,
					double proportional_gain,
					double integral_gain,
					double derivative_gain,
					double delta_t) {

	double error_derivative;
	double control;

	state->error_integrated += error * delta_t;
	error_derivative = ( error - state->error_previous ) / delta_t;
	control = error * proportional_gain
			+ state->error_integrated * integral_gain
			+ error_derivative * derivative_gain;

	state->error_previous = error ;
	return control;
}
