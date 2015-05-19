#ifndef PID_H
#define PID_H

typedef struct {
	double error_integrated;
	double error_previous ;
} pid_state_t ;

extern void initPid ( pid_state_t * state );

extern double pidUpdate ( pid_state_t *state , double error ,
							double proportional_gain ,
							double integral_gain ,
							double derivative_gain ,
							double delta_t);


#endif /* PID_H */
