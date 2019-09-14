### Writeup / README
In this project simulated RADAR and LIDAR data is used to estimate Vehicle position and Velocity using Unscented Kalman Filter. Source code provided by "SFND_Unscented_Kalman_Filter" was used as base for this project. 

### Modifications
ukf.cpp , ukf.h are modified to implement unscented kalaman filter and fusion logic. ProcessMeasurement,Prediction , UpdateLidar , UpdateRadar functions were modified to proces RADAR/ LIDAR data and to implement Unscented Kalman Filter. CalculateRMSE function was also modified to calculate RMS value. Modified code can be found [here] ("./Source")

### Code Flow
Following is brief description of the flow of the code.
- Initialize the Unscented Kalaman Filter. 
- Get the data from the simulator for one scan.
- Perdict the future position and velocity of the vehicle.
- Check if the data is from LIDAR or RADAR, accordingly call UpdateLidar or UpdateRadar functions.

## Output

