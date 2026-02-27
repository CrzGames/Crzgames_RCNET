#ifndef SERVER_H
#define SERVER_H

void rcnet_unload(void);
void rcnet_load(void);
void rcnet_network_update(void);
void rcnet_simulation_update(double dt);

#endif // SERVER_H