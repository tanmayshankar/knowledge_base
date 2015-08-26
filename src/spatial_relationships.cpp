//First define a list of objects. 
//Define the model to tag relative pose between two objects in a labelled image with all objects. 
//Run this model over the entire dataset, save the matrices generated by each. 
//Compute the relationship for the final matrix as the argmax over Values of (s_ij) of the sum of norm of ||s(ij)-p(i,j)||

//Write the final matrix to a file.


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Vector3.h>
#include <sensor_msgs/Joy.h>
#include <nav_msgs/Path.h>

#include <std_msgs/String.h>
#include <ros/console.h>
#include <cv_bridge/cv_bridge.h>
#include <ar_track_alvar/AlvarMarker.h>
#include <ar_track_alvar/AlvarMarkers.h>
#include <sstream>
#include <cmath> 

const int number_objects = 10; 
const int number_dimensions = 2; 
const int number_spatial_states = number_dimensions*2 + 2; 

double spatial_states[number_spatial_states][2]; 
double model_result[2];
// double object_centroids[number_objects][3];

double same_threshold = 0.1; 
double no_relation_threshold = 2; 
double number_training_scenes = 10; 

geometry_msgs::Vector3 object_centroids[number_objects];
geometry_msgs::PoseStamped robot_pose;

double sample_spatial_relation[number_training_scenes][number_objects][number_objects];
double spatial_relation[number_objects][number_objects];

void spatial_model(int object_i, int object_j, int current_scene)
	{	 
		 // tf::Transform trans_i;
   //       trans_i.setOrigin(tf::Vector3(-pose_marker.pose.position.x,-pose_marker.pose.position.y,-pose_marker.pose.position.z));
   //       tf::Quaternion q_i, q_f(pose_marker.pose.orientation.x, pose_marker.pose.orientation.y, pose_marker.pose.orientation.z, pose_marker.pose.orientation.w);
   //       q_i=q_f.inverse();
   //       tf::Matrix3x3 m(q_i);
         
   //       //q_i.setRPY(-roll, -pitch, -yaw);
   //       trans_i.setRotation(q_i);
   //       m.getRPY(pose_tf.angular.x,pose_tf.angular.y,pose_tf.angular.z);

         // geometry_msgs::Vector3 object_i_centroid, object_j_centroid; 
		tf::Vector3 obj_i_cent, obj_j_cent, rel_trans;
        obj_i_cent.x = object_centroids[object_i].x;
        obj_i_cent.y = object_centroids[object_i].y;
        obj_i_cent.z = object_centroids[object_i].z;
        
		obj_j_cent.x = object_centroids[object_j].x;
        obj_j_cent.y = object_centroids[object_j].y;
        obj_j_cent.z = object_centroids[object_j].z;

        rel_trans = obj_j_cent - obj_i_cent; 

        if (rel_trans.length()<same_threshold)
        	spatial_relation[object_i][object_j] = 0; 
        	
        if (rel_trans.length()>no_relation_threshold)
        	spatial_relation[object_i][object_j] = 5; 
		 
        else
         	{
         		double yaw_ir, yaw_ji, yaw;
         		yaw_ir = atan2 (object_centroids[object_i].y - robot_pose.y , object_centroids[object_i].x - robot_pose.x);
         		yaw_ji = atan2 (object_centroids[object_j].y - object_centroids[object_i].y , object_centroids[object_j].x - object_centroids[object_i].x);
         		yaw = yaw_ji - yaw_ir + 3.14; 
         		yaw *= 57.32;
         		if ((yaw<135)&&(yaw>45))
         			sample_spatial_relation[current_scene][object_i][object_j] = 3; //Front. 
         		if ((yaw<-45)&&(yaw>-135))
         			sample_spatial_relation[current_scene][object_i][object_j] = 4; //Behind. 
         		if ((yaw>135)||(yaw<-135))
         			sample_spatial_relation[current_scene][object_i][object_j] = 2; //Right. 
         		if ((yaw<45)&&(yaw>-45))
         			sample_spatial_relation[current_scene][object_i][object_j] = 1; //Left. 
         	}

	}

		// void spatial_model(double x_i, double x_j, double y_i, double y_j)
	// {	model_result[0] = abs(x_i-x_j) / (x_i-x_j);
		// model_result[1] = abs(y_i-y_j) / (y_i-y_j);
	// }

void init_spatial_states()
	{	
		// for (int i=0; i<number_spatial_states; i++)

		//Not correlated. 
		spatial_states[5][0] = 0; 
		spatial_states[5][1] = 0; 

		//Same. 
		spatial_states[0][0] = 1; 
		spatial_states[0][1] = 1; 

		//Left. 
		spatial_states[1][0] = 1; 
		spatial_states[1][1] = 0; 

		//Right. 
		spatial_states[2][0] = -1; 
		spatial_states[2][1] = 0; 

		//Front. 
		spatial_states[3][0] = 0; 
		spatial_states[3][1] = 1; 

		//Behind. 
		spatial_states[4][0] = 0; 
		spatial_states[4][1] = -1; 

	}

void compute_sample_relationships()
	{	// Load a point cloud file, run the detector, store the object centroids, 
		//Then call spatial_model over each pair of objects
		//Store / save the results into whatever 

	}

void compute_relationship_matrix()
	{	double value_rel, max_value=0;
		int max_index = 0; 
		tf::Vector3 temp;
		for (int i=0; i<number_objects; i++)
			{	for (int j=0; j<number_objects; j++)
					{	if (i==j)
							{	spatial_relation[i][j] = 0;		//Same. 
								continue; 
							}
						if (j<=i)
							continue; 
						for (int p=0; p<number_spatial_states; p++)
							{	value_rel=0; 
								for (int k=0; k<number_training_scenes; k++)
									{	temp.x = spatial_states[p][0] - spatial_states[sample_spatial_relation[k][i][j]][0]; 
										temp.y = spatial_states[p][1] - spatial_states[sample_spatial_relation[k][i][j]][1]; 
										temp.z = 0; 
										value_rel += temp.length;  						
									}							
								if (value_rel>max_value)
									{	max_index=p;
										max_value=value_rel; 
									}
							}
						spatial_relation[i][j] = max_index;
						spatial_relation[j][i] = max_index + pow(-1,max_index+1);
					}
			}	
	}

int main()
	{	return 0; 
	}





int main(int argc, char** argv)
{
  //Initialize the node. 
  ros::init(argc, argv, "pub_mav_follow");
  


  setpoint setpoint_ob;


  ros::spin();
}


