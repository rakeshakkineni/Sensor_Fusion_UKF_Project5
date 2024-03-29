#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using Eigen::MatrixXd;
using Eigen::VectorXd;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 0.7;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.9;
  
  /**
   * DO NOT MODIFY measurement noise values below.
   * These are provided by the sensor manufacturer.
   */

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  
  /**
   * End DO NOT MODIFY section for measurement noise values 
   */
  //Initialize n_x_ to 5 as five state parameters are predicted
   n_x_ = 5;

   //Initialize n_aug_ to 7 as augmented matrix of size 7
   n_aug_ =7;

  is_initialized_ =false;
  /**
   * TODO: Complete the initialization. See ukf.h for other member properties.
   * Hint: one or more values initialized above might be wildly off...
   */
  //Xsig_pred holds 15 points of the state for transformation
  Xsig_pred_ = MatrixXd(n_x_, 2*n_aug_+1);
}

UKF::~UKF() {}

void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Make sure you switch between lidar and radar
   * measurements.
   */
  	if (!is_initialized_) {

		// first measurement
		//std::cout << "UKF: " << std::endl;

		//Initialize P with identity matrix
		P_.setIdentity(5,5);
		P_(2,2)= 10;
		P_(3,3)= 50;
		P_(4,4)= 3;

		lambda_ = 3 -n_aug_;

		weights_ = VectorXd(2*n_aug_+1);

		// set weights they remain constant throughout the processes
		double weight_0 = lambda_/(lambda_+n_aug_);
		weights_(0) = weight_0;
		for (int i=1; i<2*n_aug_+1; i++)
		{  //2n+1 weights
			double weight = 0.5/(n_aug_+lambda_);
			weights_(i) = weight;
		}

		if ((meas_package.sensor_type_ == MeasurementPackage::RADAR)) {
		  /**
		  Convert radar from polar to cartesian coordinates and initialize state.
		  */
		  x_ << meas_package.raw_measurements_[0]*cos(meas_package.raw_measurements_[1]),meas_package.raw_measurements_[0]*sin(meas_package.raw_measurements_[1]), 0, 0, 0;
		}
		else if ((meas_package.sensor_type_ == MeasurementPackage::LASER)) {
		  /**
		  Initialize state.
		  */
		  //set the state with the initial location and zero velocity
		  x_ << meas_package.raw_measurements_[0], meas_package.raw_measurements_[1], 0, 0, 0;

		}

		time_us_ = meas_package.timestamp_;

		//outputfile.open("nis_output.txt",ios::app|ios::ate);
        Xsig_pred_.fill(0.0);

		// done initializing, no need to predict or update
		is_initialized_ = true; 

		return;
	}
    
    //compute the time elapsed between the current and previous measurements
	double dt = (meas_package.timestamp_ - time_us_) / 1000000.0;	//dt - expressed in seconds
	time_us_ = meas_package.timestamp_;

   /*****************************************************************************
   *  Prediction
   ****************************************************************************/

    Prediction(dt);
	
    /*****************************************************************************
	*  Update
	****************************************************************************/

	if ((meas_package.sensor_type_ == MeasurementPackage::RADAR)&&(use_radar_==true)) {
	// Radar updates
	   //std::cout<<"Call RadarUpdate "<< std::endl;
	   UpdateRadar(meas_package);


	} else if ((meas_package.sensor_type_ == MeasurementPackage::LASER)&&(use_laser_==true)){
		//std::cout<<"Call LidarUpdate "<< std::endl;
	  UpdateLidar(meas_package);
	}

//	std::cout<<"Init_State: "<<is_initialized_<<std::endl;
}

void UKF::Prediction(double delta_t) {
  /**
   * TODO: Complete this function! Estimate the object's location. 
   * Modify the state vector, x_. Predict sigma points, the state, 
   * and the state covariance matrix.
   */
  // Augmented sigma points matrix
  MatrixXd Xsig_aug = MatrixXd(7, 15);
  Xsig_aug.fill(0);
  //std::cout<<"Predict"<<std::endl;

  // Find the augmented sigma points
  AugmentedSigmaPoints(&Xsig_aug);
  // Sigma point transformation using the process equation
  SigmaPointPrediction(&Xsig_aug,delta_t);
  // Mean and Covariance prediction of the transformed sigma points
  PredictMeanAndCovariance();
}

/**
 * Generates the augmeneted sigma points.
 * @param {MatrixXd} Xsig_out : Augmented sigma points
 */
void UKF::AugmentedSigmaPoints(MatrixXd* Xsig_out) {

  //create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);

  //create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);

  //create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

  //create augmented mean state
  x_aug.fill(0.0);
  x_aug.head(5) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;


  //create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5,5) = std_a_*std_a_;
  P_aug(6,6) = std_yawdd_*std_yawdd_;

  //create square root matrix
  MatrixXd L = P_aug.llt().matrixL();

  //create augmented sigma points
  Xsig_aug.fill(0.0);
  Xsig_aug.col(0)  = x_aug;

  for (int i = 0; i< n_aug_; i++)
  {
    Xsig_aug.col(i+1)       = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);

  }

  //print result
  //std::cout << "Xsig_aug = " << std::endl << Xsig_aug << std::endl;

  //write result
  *Xsig_out = Xsig_aug;

}

/**
 * Transforms the augmeneted sigma points using the process equations.
 * @param {MatrixXd*} aug : Augmented sigma points
 * 		  {double }	delta_t: Time difference
 */

void UKF::SigmaPointPrediction(MatrixXd* aug, double delta_t) {

  //create matrix with predicted sigma points as columns
  MatrixXd Xsig_pred = MatrixXd(n_x_, 2 * n_aug_ + 1);
  //create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

  Xsig_pred.fill(0.0);
  Xsig_aug = *aug;
  //predict sigma points

  for (int i = 0; i< 2*n_aug_+1; i++)
  {
    //extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);

    //predicted state values
    double px_p, py_p;

    //avoid division by zero
    if (fabs(yawd) > 0.001) {
        px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
        py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
    }
    else {
        px_p = p_x + v*delta_t*cos(yaw);
        py_p = p_y + v*delta_t*sin(yaw);
    }

    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;

    //add noise
    px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
    py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
    yawd_p = yawd_p + nu_yawdd*delta_t;

    //write predicted sigma point into right column
    Xsig_pred(0,i) = px_p;
    Xsig_pred(1,i) = py_p;
    Xsig_pred(2,i) = v_p;
    Xsig_pred(3,i) = yaw_p;
    Xsig_pred(4,i) = yawd_p;
  }

  //write result
  Xsig_pred_ = Xsig_pred;
  //std::cout << "Xsig_pred_ = " << std::endl << Xsig_pred_ << std::endl;
}

/**
 * Calculate the mean and covariance using the augmented sigma points.
 * @param void
 */

void UKF::PredictMeanAndCovariance(void) {

//create vector for predicted state
VectorXd x = VectorXd(n_x_);

//create covariance matrix for prediction
MatrixXd P = MatrixXd(n_x_, n_x_);


//predicted state mean
x.fill(0.0);
P.fill(0.0);
for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points

x = x+ weights_(i) * Xsig_pred_.col(i);

}

//predicted state covariance matrix
P.fill(0.0);
for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points

// state difference
VectorXd x_diff = Xsig_pred_.col(i) - x;
//angle normalization
while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

P = P + weights_(i) * x_diff * x_diff.transpose() ;
}

//print result
/*std::cout << "Predicted state" << std::endl;
std::cout << x << std::endl;
std::cout << "Predicted covariance matrix" << std::endl;
std::cout << P << std::endl;*/

//write result
x_ = x;
P_ = P;
//std::cout << "State = " << x_[0] << std::endl;
}

void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use lidar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the lidar NIS, if desired.
   */
    VectorXd z_out = VectorXd(2);
	MatrixXd S_out = MatrixXd(2, 2);
	MatrixXd z_sig = MatrixXd(2, 2 * n_aug_ + 1);

	z_out.fill(0.0);
	S_out.fill(0.0);
	z_sig.fill(0.0);
	//std::cout<<"LidarUpdate "<< std::endl;
	// Predict the Laser Measurements before updating with new measurements
    PredictLidarMeasurement(&z_out, &z_sig, &S_out);
	// Update the Lidar state using the new measurements
	UpdateStateLidar(z_out, z_sig, S_out, meas_package);
}
/**
 * Update Lidar measurements with new measurements.
 * @param {VectorXd*} z_pred:mean predicted measurement
 * 		  {MatrixXd*} Zsig:transformed sigma points into measurement space
 * 		  {MatrixXd*} S:innovation covariance matrix S
 * 		  {MeasurementPackage*} meas_package:New measurement points
 */

void UKF::UpdateStateLidar(VectorXd &z_pred, MatrixXd &Zsig,MatrixXd &S, MeasurementPackage meas_package) {

  //create example vector for incoming radar measurement
  VectorXd z = VectorXd(2);
  z.fill(0.0);
  z <<meas_package.raw_measurements_[0],
      meas_package.raw_measurements_[1];

  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, 2);

/*******************************************************************************
 * Student part begin
 ******************************************************************************/

  //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    //angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  //residual
  VectorXd z_diff = z - z_pred;

  //angle normalization
  while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K*S*K.transpose();

/*******************************************************************************
 * Student part end
 ******************************************************************************/

  //print result
  //std::cout << "Updated state x: " << std::endl << x_ << std::endl;
  //std::cout << "Updated state covariance P: " << std::endl << P_ << std::endl;

  //laser_nis_ = z_diff.transpose()*S.inverse()*z_diff;
  //outputfile.seekp(0,std::ios::end);
  //outputfile <<"L\t"<<laser_nis_<<"\t"<<std::endl;

}

void UKF::PredictLidarMeasurement(VectorXd* z_out,MatrixXd* z_sig, MatrixXd* S_out) {

  //set measurement dimension, radar can measure r, phi, and r_dot
  int n_z = 2;
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
 //create example matrix with predicted sigma points


/*******************************************************************************
 * Student part begin
 ******************************************************************************/
  Zsig.fill(0.0);
  //transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    // extract values for better readibility
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    // measurement model
    Zsig(0,i) = p_x;                        //r
    Zsig(1,i) = p_y;                                 //phi

  }

  //mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; i++) {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
  }

  //innovation covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<    std_laspx_*std_laspx_, 0,
          0, std_laspy_*std_laspy_ ;
  S = S + R;


/*******************************************************************************
 * Student part end
 ******************************************************************************/

  //print result
 // std::cout << "z_pred: " << std::endl << z_pred << std::endl;
 // std:: << "S: " << std::endl << S << std::endl;

  //write result
  *z_out = z_pred;
  *z_sig = Zsig;
  *S_out = S;
}

void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use radar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the radar NIS, if desired.
   */
    VectorXd z_out = VectorXd(3);
	MatrixXd S_out = MatrixXd(3, 3);
	MatrixXd z_sig = MatrixXd(3, 2 * n_aug_ + 1);

	// Predict the Radar Measurements before updating with new measurements
	PredictRadarMeasurement(&z_out, &z_sig, &S_out);
	// Update the Radar state using the new measurements
	UpdateStateRadar(z_out, z_sig, S_out, meas_package);
}
/**
 * Predict the Radar measurement before updating with new measurements.
 * @param {VectorXd*} z_out:mean predicted measurement
 * 		  {MatrixXd*} z_sig:transformed sigma points into measurement space
 * 		  {MatrixXd*} S_out:innovation covariance matrix S
 */

void UKF::PredictRadarMeasurement(VectorXd* z_out,MatrixXd* z_sig, MatrixXd* S_out) {

  //set measurement dimension, radar can measure r, phi, and r_dot
  int n_z = 3;
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  Zsig.fill(0.0);
 //create example matrix with predicted sigma points

/*******************************************************************************
 * Student part begin
 ******************************************************************************/

  //transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    // extract values for better readibility
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;

    // measurement model
    Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                        //r
   // std::cout<<"RadarUpdate 0"<< std::endl;
    //check division by zero
    /*
    if(Zsig(0,i) < 0.0001){
    	cout << "CalculateJacobian () - Error - Division by Zero" << endl;
    	return;
    }*/
    Zsig(1,i) = atan2(p_y,p_x);                                 //phi
    Zsig(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);   //r_dot
   // std::cout<<"RadarUpdate 1"<< std::endl;
  }

  //mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; i++) {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
  }
  //std::cout<<"RadarUpdate 2"<< std::endl;
  //innovation covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }
  //std::cout<<"RadarUpdate 3"<< std::endl;
  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<    std_radr_*std_radr_, 0, 0,
          0, std_radphi_*std_radphi_, 0,
          0, 0,std_radrd_*std_radrd_;
  S = S + R;
  //std::cout<<"RadarUpdate 4"<< std::endl;

/*******************************************************************************
 * Student part end
 ******************************************************************************/

  //print result
  //std::cout << "z_pred: " << std::endl << z_pred << std::endl;
  //std::cout << "S: " << std::endl << S << std::endl;

  //write result
  *z_out = z_pred;
  *z_sig = Zsig;
  *S_out = S;

  //std::cout<<"RadarUpdate 5"<< std::endl;
}

/**
 * Update Radar measurements with new measurements.
 * @param {VectorXd*} z_pred:mean predicted measurement
 * 		  {MatrixXd*} Zsig:transformed sigma points into measurement space
 * 		  {MatrixXd*} S:innovation covariance matrix S
 * 		  {MeasurementPackage*} meas_package:New measurement points
 */

void UKF::UpdateStateRadar(VectorXd &z_pred, MatrixXd &Zsig,MatrixXd &S, MeasurementPackage meas_package) {

  //create example vector for incoming radar measurement

  VectorXd z = VectorXd(3);
  z <<meas_package.raw_measurements_[0],
      meas_package.raw_measurements_[1],
	  meas_package.raw_measurements_[2];

/*  z << 5.9124,
		  0.2187,
		  2.0062;*/

  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, 3);

/*******************************************************************************
 * Student part begin
 ******************************************************************************/

  //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    //angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  //residual
  VectorXd z_diff = z - z_pred;

  //angle normalization
  while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K*S*K.transpose();

/*******************************************************************************
 * Student part end
 ******************************************************************************/

  //print result
 // std::cout << "Updated state x: " << std::endl << x_ << std::endl;
 // std::cout << "Updated state covariance P: " << std::endl << P_ << std::endl;

  //radar_nis_ = z_diff.transpose()*S.inverse()*z_diff;

  //outputfile.seekp(0,std::ios::end);
  //outputfile <<"R\t"<<radar_nis_<<"\t"<<std::endl;

}
