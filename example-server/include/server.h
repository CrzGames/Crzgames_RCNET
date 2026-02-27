#ifndef SERVER_H
#define SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

void rcnet_unload(void);
void rcnet_load(void);
void rcnet_network_update(void);
void rcnet_simulation_update(double dt);

#ifdef __cplusplus
}
#endif

#endif // SERVER_H