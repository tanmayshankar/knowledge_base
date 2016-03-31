// This code is used to extract relative pose information between pairs of objects in a labelled point cloud. 
// We first parse the pointcloud, and define a list of objects according to the number of distinct objects present in the pointcloud. 

// We then define a model to tag relative pose between two objects in a labelled pointcloud with all objects. 
// We then run this model over the entire dataset, and save the matrices generated by each instance. 

// The final spatial relationships are defined as the mean and variance of the distribution
// that minimizes the L2 norm of centroid distances between objects across all scenes / pointclouds. 

// Finally, the resultant spatial relationships (mean and variance) are written to a file for later access. 

#include <ros/ros.h>

// PCL Headers
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/ModelCoefficients.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/passthrough.h>
#include <pcl/features/normal_3d.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/ModelCoefficients.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/kdtree/kdtree.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>

// Including custom point type. 
#include "includes/custom_point_types.h"

// Additional header files. 
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Vector3.h>
#include <sensor_msgs/Joy.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>

#include <tf/transform_broadcaster.h>
#include <tf/transform_datatypes.h>
#include <std_msgs/String.h>
#include <ros/console.h>
#include <cv_bridge/cv_bridge.h>
#include <sstream>
#include <vector>
#include <cmath> 

// Defining training dataset parameters. 
const int number_objects = 130; 
const int number_dimensions = 2; 
const int number_training_scenes = 10; 

// Defining global variables pertaining to PCL. 
typedef pcl::PointXYZRGBCamSL PointT;
pcl::PointCloud<PointT>::Ptr cloud (new pcl::PointCloud<PointT>);
// pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);

// Defining threshold and default values for our algorithm.
double same_threshold = 0.1; 
double default_deviation=1000000;

// Declaring global variables of objects. 
geometry_msgs::Vector3 object_centroids[number_objects];
geometry_msgs::PoseStamped robot_pose;

// Declaring spatial object relationship matrices. 
double sample_spatial_relation[number_training_scenes][number_objects][number_objects];
double spatial_relation_mean[number_objects][number_objects];
double spatial_relation_dev[number_objects][number_objects];

int object_occurance[number_objects];

// Defining object class. 
class objects
	{	
		// Defining public data elements of object class. 
		public: 
			int label_number, number_points; 
			tf::Vector3 centroid; 
			geometry_msgs::Vector3 geo_centroid;

			//Default constructor of object class to initialize objects. 

			objects()
				{	label_number=0;
					number_points=0;  
					geo_centroid.x = 0; 
					geo_centroid.y = 0; 
					geo_centroid.z = 0; 
				}

			//Overloading << operator to take data from pointcloud scene s to the object elements. 
			friend std::ostream& operator<<(std::ostream& os, const objects& obj_op_ptr)
				{			
					// Write out individual members of s, with an end of line between each object. 
					os<<obj_op_ptr.label_number<<std::endl; 
					os<<obj_op_ptr.number_points<<std::endl;
					os<<obj_op_ptr.geo_centroid;
					return os;
				} 

			// Overloading extraction operator
			// friend std::istream& operator>>(std::istream& is, const& obj_ip_ptr)
			// 	{
			// 		// read in individual members of scene_ip_ptr					
			// 		is>>obj_ip_ptr.label_number; 
			// 		is>>obj_ip_ptr.number_points;
			// 		is>>obj_ip_ptr.geo_centroid;
			// 		return is;
			// 	}
	};

//Defining the scene class. 
class scenes
	{	
		//Defining public data elements. 
		public: 
			
			// Vector of objects of 'object' class. 
			std::vector<objects> scene_obj_vector;

			int scene_num_obj;
			double scene_spatial_rel[number_objects][number_objects];
			
			//Default constructor. 
			scenes()
				{	
					// Declaring object class instance, and pushing into the scene vector.
					objects dummy_obj; 
	
					scene_obj_vector.push_back(dummy_obj);
					scene_num_obj = 0; 
				}
	};

//Defining a vector of scenes. 
std::vector<scenes> scene_vector; 

//Function definition to compute relationships present in the current scene. 
void compute_sample_relationships()
	{	

		// Load a point cloud file, run the detector, store the object centroids, 
		//Then call spatial_model over each pair of objects
		//Store or save the results into whatever 
		
		for (int scene_ind=0; scene_ind<scene_vector.size(); scene_ind++)			
			for (int i=0; i<number_objects; i++)					
				for (int j=0; j<number_objects; j++)
					scene_vector[scene_ind].scene_spatial_rel[i][j] = -1;	
			
		for (int scene_ind=0; scene_ind<scene_vector.size(); scene_ind++)
			for (int obj_ind_i=0; obj_ind_i<scene_vector[scene_ind].scene_obj_vector.size(); obj_ind_i++)
				for (int obj_ind_j=0; obj_ind_j<scene_vector[scene_ind].scene_obj_vector.size(); obj_ind_j++)
					{	
						tf::Vector3 rel_trans(scene_vector[scene_ind].scene_obj_vector[obj_ind_i].geo_centroid.x - scene_vector[scene_ind].scene_obj_vector[obj_ind_j].geo_centroid.x,
									  		  scene_vector[scene_ind].scene_obj_vector[obj_ind_i].geo_centroid.y - scene_vector[scene_ind].scene_obj_vector[obj_ind_j].geo_centroid.y,
   									  		  // scene_vector[scene_ind].scene_obj_vector[obj_ind_i].geo_centroid.z - scene_vector[scene_ind].scene_obj_vector[obj_ind_j].geo_centroid.z);					
											  0);   		   									  		  
	
						//Change from hard decision about threshold distance. 
						//Now it just notes the distance by which they are related if the co-occur in the scene at all. 
						//The Gaussian in Querying spatial relationships takes care of this. 
						//If the object with that particular label isn't present in the scene, the mean value is negative as per the above loop.

						int x23 = scene_vector[scene_ind].scene_obj_vector[obj_ind_i].label_number;
						int x45 = scene_vector[scene_ind].scene_obj_vector[obj_ind_j].label_number;
						// std::cout<<"I Label number: "<<x23<<" J Label number: "<<x45<<std::endl;
						// scene_vector[scene_ind].scene_spatial_rel[scene_vector[scene_ind].scene_obj_vector[obj_ind_i].label_number][scene_vector[scene_ind].scene_obj_vector[obj_ind_j].label_number] = rel_trans.length();
						scene_vector[scene_ind].scene_spatial_rel[x23][x45] = rel_trans.length();
					}							
	}


//Function definition to compute overall spatial relationships over all scenes and objects. 
void compute_relationship_matrix()
	{	
		//Defining local variables.
		int num_training_scenes = scene_vector.size();
		int num_corr_objs;
		int num_obs=10; 
		double mean_value=0;
		double dev_value=0;

		//Initializing the spatial relationship matrices. 
		for (int i=0; i<number_objects; i++)
			for (int j=0; j<number_objects; j++)
				{	spatial_relation_mean[i][j]=-1;
					spatial_relation_dev[i][j]=default_deviation;
				}					

		// For all object pairs present, compute the mean radius and variance that minimizes the L2 norm error across all scenes. 
		for (int i=0; i<number_objects; i++)
			for (int j=0; j<number_objects; j++)
				{	

					num_corr_objs=0;					
					mean_value=0;
					dev_value=0;

					if (j<i)
						continue; 										

					// For every scene present, update mean value.
					for (int k=0; k<scene_vector.size(); k++)
						if (scene_vector[k].scene_spatial_rel[i][j]>0)
							{	mean_value += scene_vector[k].scene_spatial_rel[i][j];
								num_corr_objs++;
							}					

					// Averaging mean values.
					if (mean_value!=0)
						{	spatial_relation_mean[i][j] = mean_value/num_corr_objs;
							spatial_relation_mean[j][i] = mean_value/num_corr_objs;
						}

					// Compute the variance of the distribution. 
					for (int k=0; k<scene_vector.size(); k++)
						if (scene_vector[k].scene_spatial_rel[i][j]>0)
							dev_value += pow(scene_vector[k].scene_spatial_rel[i][j] - spatial_relation_mean[i][j] ,2);

					// Averaging variance values. 
					if (dev_value!=0)
						{	
							// std::cout<<"Hello4343"<<std::endl;
							spatial_relation_dev[i][j] = sqrt(dev_value/num_corr_objs);
							spatial_relation_dev[j][i] = sqrt(dev_value/num_corr_objs);
						}						
				}				
	}

void parse_dataset(int number_files, char** file_pointer)
	{	
		//Defining managing variables. 
		int file_index;
		int flag; 
		pcl::PCDReader reader;

		// Defining temporary object. 
		objects temp_obj; 

		// Defining temporary scene. 
		scenes temp_scene; 
				
		//Iterating over all files (pointclouds). 
		for (int file_ind=1; file_ind<number_files-1; file_ind++)
			{	
				// std::cout<<"File number: "<<file_ind-1<<std::endl;

				//Using PCL pointcloud reader to input the pointcloud file. This is templated over particular pointcloud type. 
				reader.read (file_pointer[file_ind], *cloud);

				// Dummy Zero indexing. 
				file_index = file_ind - 1; 
				
				// Object count in a particular scene.
				for (int i=0; i<number_objects; i++)
					object_occurance[i]=0; 

				// Iterating over all points in the point cloud. 
				// For every point, we check the label, and assign it to a new object if it is unique. 
				// If it isn't unique, update the corresponding model data with this point information, and update centroid of the object. 
				
				for (size_t pt_index=0; pt_index<cloud->size(); pt_index++)			
					{	
				
						flag=0; 
				
						//For all objects in the current scene.
						for (int vec_index=0; vec_index<temp_scene.scene_obj_vector.size(); vec_index++)

							//If the point label coincides with one of the objets previously detected in the scene. 
							if (cloud->points[pt_index].label == temp_scene.scene_obj_vector[vec_index].label_number)
							
								{										
									// Update the centroid of the corresponding object. 
									temp_scene.scene_obj_vector[vec_index].geo_centroid.x += cloud->points[pt_index].x; 
									temp_scene.scene_obj_vector[vec_index].geo_centroid.y += cloud->points[pt_index].y;
									temp_scene.scene_obj_vector[vec_index].geo_centroid.z += cloud->points[pt_index].z; 
									temp_scene.scene_obj_vector[vec_index].number_points++; 

									// Recomputing averages for centroid. 
									int np = temp_scene.scene_obj_vector[vec_index].number_points; 
									temp_scene.scene_obj_vector[vec_index].geo_centroid.x = (temp_scene.scene_obj_vector[vec_index].geo_centroid.x)/np;
									temp_scene.scene_obj_vector[vec_index].geo_centroid.y = (temp_scene.scene_obj_vector[vec_index].geo_centroid.y)/np;
									temp_scene.scene_obj_vector[vec_index].geo_centroid.z = (temp_scene.scene_obj_vector[vec_index].geo_centroid.z)/np;
									
									flag=1;							
								}
								
						// If the object wasn't present so far, add new object. 
						if (flag==0)
							{	
								// Initializing temporary object with details of the object label of this point. 
								temp_obj.label_number = cloud->points[pt_index].label;
								temp_obj.number_points = 1; 
								temp_obj.geo_centroid.x = cloud->points[pt_index].x; 
								temp_obj.geo_centroid.y = cloud->points[pt_index].y; 
								temp_obj.geo_centroid.z = cloud->points[pt_index].z; 						
								temp_scene.scene_num_obj++;

								// Pushing the new object into the current scene vector. 
								temp_scene.scene_obj_vector.push_back(temp_obj);

								// Adding to the count of the this object. 
								object_occurance[temp_obj.label_number]=1;
								scene_vector[file_index].scene_num_obj++;

								// Pushing the new object into the overall scene vector. 
								scene_vector[file_index].scene_obj_vector.push_back(temp_obj);
							}
					}

				
				// Printing the dataset details into a file corresponding to the input pointcloud. 
				char buffer[32];
				snprintf(buffer, sizeof(char) * 32, "file%i.txt", file_index);						
				std::ofstream ofs(buffer);

				//USE THE FOLLOWING 4 LINES TO WRITE TO FILE A BINARY VARIABLE AS TO WHETHER THE OBJECT WITH THAT INDEX OCCURS IN A PARTICULAR SCENE OR NOT			

				for (int ob=0; ob<number_objects; ob++)
				 	ofs<<object_occurance[ob]<<std::endl;
					
				//USE THESE LINES TO WRITE TO THE FILE : 
				/*	FOR EACH OBJECT PRESENT IN THE SCENE,
					LABEL NUMBER
					NUMBER OF POINTS
					GEOMETRIC CENTROID OF THAT POINT
				*/
				// Store the object to file						
				// for (int vec_index=0; vec_index<temp_scene.scene_obj_vector.size(); vec_index++)
				// 	ofs<<temp_scene.scene_obj_vector[vec_index]<<std::endl; 

				// Closing file object. 
				ofs.close();
				
			}
	}


//Main. 
int main(int argc, char** argv)
	{	
		
		//Parsing the set of files / dataset, and calculating spatial relationships.
		parse_dataset(argc,argv);	
		
		return 0; 
	}