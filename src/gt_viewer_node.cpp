/* gt_viewer_node.cpp

  Copyright (C) 2021 SS47816 & Advanced Robotics Center, National University of Singapore

  LGSVL Simulator 3D Objects Ground Truth Viewer ROS Node
*/

#include <iostream>
#include <string>
#include <vector>

#include <ros/ros.h>
#include <ros/console.h>

#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <geometry_msgs/PoseStamped.h>
#include <lgsvl_msgs/Detection3D.h>
#include <lgsvl_msgs/Detection2DArray.h>
#include <lgsvl_msgs/Detection3DArray.h>
#include <lgsvl_msgs/BoundingBox3D.h>
#include <jsk_recognition_msgs/BoundingBox.h>
#include <jsk_recognition_msgs/BoundingBoxArray.h>
#include <autoware_msgs/DetectedObjectArray.h>

namespace lgsvl_utils 
{

class GTViwerNode
{
 public:
  GTViwerNode();
  virtual ~GTViwerNode() {};

 private:
  ros::NodeHandle nh;
  ros::Subscriber lgsvl_gt2d_sub;
  ros::Subscriber lgsvl_gt3d_sub;
  ros::Publisher jsk_bboxes_pub;
  ros::Publisher autoware_objects_pub;

  tf2_ros::Buffer tf2_buffer;
  tf2_ros::TransformListener tf2_listener;

  std::string bbox_target_frame_;
  double bbox_filter_size_;
  
  void detections2DCallback(const lgsvl_msgs::Detection2DArray::ConstPtr& lgsvl_detections2d);
  void detections3DCallback(const lgsvl_msgs::Detection3DArray::ConstPtr& lgsvl_detections3d);
  jsk_recognition_msgs::BoundingBox transformJskBbox(const lgsvl_msgs::Detection3D::ConstPtr& lgsvl_detection3d, const geometry_msgs::Pose& pose_transformed);
  autoware_msgs::DetectedObject transformAutowareObject(const lgsvl_msgs::Detection3D::ConstPtr& lgsvl_detection3d, const geometry_msgs::Pose& pose_transformed);
};

GTViwerNode::GTViwerNode() : tf2_listener(tf2_buffer)
{
  ros::NodeHandle private_nh("~");
  
  std::string lgsvl_gt2d_topic;
  std::string lgsvl_gt3d_topic;
  std::string jsk_bboxes_topic;
  std::string autoware_objects_topic;
  
  ROS_ASSERT(private_nh.getParam("lgsvl_gt2d_topic", lgsvl_gt2d_topic));
  ROS_ASSERT(private_nh.getParam("lgsvl_gt3d_topic", lgsvl_gt3d_topic));
  ROS_ASSERT(private_nh.getParam("jsk_bboxes_topic", jsk_bboxes_topic));
  ROS_ASSERT(private_nh.getParam("autoware_objects_topic", autoware_objects_topic));
  ROS_ASSERT(private_nh.getParam("bbox_target_frame", bbox_target_frame_));
  ROS_ASSERT(private_nh.getParam("bbox_filter_size", bbox_filter_size_));

  lgsvl_gt2d_sub = nh.subscribe(lgsvl_gt2d_topic, 1, &GTViwerNode::detections2DCallback, this);
  lgsvl_gt3d_sub = nh.subscribe(lgsvl_gt3d_topic, 1, &GTViwerNode::detections3DCallback, this);
  jsk_bboxes_pub = nh.advertise<jsk_recognition_msgs::BoundingBoxArray>(jsk_bboxes_topic, 1);
  autoware_objects_pub = nh.advertise<autoware_msgs::DetectedObjectArray>(autoware_objects_topic, 1);
}

void GTViwerNode::detections2DCallback(const lgsvl_msgs::Detection2DArray::ConstPtr& lgsvl_detections2d)
{
  lgsvl_detections2d->header;
}

jsk_recognition_msgs::BoundingBox GTViwerNode::transformJskBbox(const lgsvl_msgs::Detection3D::ConstPtr& lgsvl_detection3d, const geometry_msgs::Pose& pose_transformed)
{
  jsk_recognition_msgs::BoundingBox jsk_bbox;
  jsk_bbox.header = lgsvl_detection3d->header;
  jsk_bbox.header.frame_id = bbox_target_frame_;
  jsk_bbox.pose = pose_transformed;
  jsk_bbox.pose.position.z += lgsvl_detection3d->bbox.size.z/2;
  jsk_bbox.dimensions = lgsvl_detection3d->bbox.size;
  jsk_bbox.value = lgsvl_detection3d->score;
  if (lgsvl_detection3d->label == "Pedestrian")
  { 
    jsk_bbox.label = 2;
  }
  else if (lgsvl_detection3d->label == "Bicyclist")
  { 
    jsk_bbox.label = 1;
  }
  else
  { 
    jsk_bbox.label = 0;
  }

  return std::move(jsk_bbox);
}

autoware_msgs::DetectedObject GTViwerNode::transformAutowareObject(const lgsvl_msgs::Detection3D::ConstPtr& lgsvl_detection3d, const geometry_msgs::Pose& pose_transformed)
{
  autoware_msgs::DetectedObject autoware_object;
  autoware_object.header = lgsvl_detection3d->header;
  autoware_object.header.frame_id = bbox_target_frame_;
  autoware_object.id = lgsvl_detection3d->id;
  autoware_object.label = lgsvl_detection3d->label;
  autoware_object.score = lgsvl_detection3d->score;
  autoware_object.pose = pose_transformed;
  autoware_object.pose.position.z += lgsvl_detection3d->bbox.size.z/2;
  autoware_object.pose_reliable = true;
  autoware_object.dimensions = lgsvl_detection3d->bbox.size;
  autoware_object.velocity = lgsvl_detection3d->velocity;
  autoware_object.velocity_reliable = true;
  autoware_object.valid = true;
  if (lgsvl_detection3d->label == "Pedestrian")
  { 
    autoware_object.label = "person";
  }
  else if (lgsvl_detection3d->label == "Bicyclist")
  { 
    autoware_object.label = "bicycle";
  }
  else if (lgsvl_detection3d->label == "SchoolBus")
  { 
    autoware_object.label = "bus";
  }
  else if (lgsvl_detection3d->label == "BoxTruck")
  { 
    autoware_object.label = "truck";
  }
  else 
  { 
    autoware_object.label = "car";
  }

  return std::move(autoware_object);
}

void GTViwerNode::detections3DCallback(const lgsvl_msgs::Detection3DArray::ConstPtr& lgsvl_detections3d)
{
  geometry_msgs::TransformStamped transform_stamped;
  try
  {
    transform_stamped = tf2_buffer.lookupTransform(bbox_target_frame_, lgsvl_detections3d->header.frame_id, ros::Time(0));
  }
  catch (tf2::TransformException& ex)
  {
    ROS_WARN("%s", ex.what());
    return;
  }

  jsk_recognition_msgs::BoundingBoxArray jsk_bboxes;
  jsk_bboxes.header = lgsvl_detections3d->header;
  jsk_bboxes.header.frame_id = bbox_target_frame_;
  autoware_msgs::DetectedObjectArray autoware_objects;
  autoware_objects.header = lgsvl_detections3d->header;
  autoware_objects.header.frame_id = bbox_target_frame_;
  
  for (const auto& lgsvl_detection3d : lgsvl_detections3d->detections)
  {
    // Filter out false objects from lgsvl
    if (lgsvl_detection3d.bbox.size.z < bbox_filter_size_)
    {
      geometry_msgs::Pose pose_transformed;
      tf2::doTransform(lgsvl_detection3d.bbox.position, pose_transformed, transform_stamped);
      auto lgsvl_detection3d_ptr = boost::make_shared<lgsvl_msgs::Detection3D>(lgsvl_detection3d);
      jsk_bboxes.boxes.emplace_back(transformJskBbox(lgsvl_detection3d_ptr, pose_transformed));
      autoware_objects.objects.emplace_back(transformAutowareObject(lgsvl_detection3d_ptr, pose_transformed));
    }
  }
  jsk_bboxes_pub.publish(jsk_bboxes);
  autoware_objects_pub.publish(autoware_objects);
}

} // namespace lgsvl_utils

int main(int argc, char** argv)
{
  ros::init(argc, argv, "gt_viewer_node");
  lgsvl_utils::GTViwerNode gt_viewer_node;
  ros::spin();  // spin the ros node.
  return 0;
}