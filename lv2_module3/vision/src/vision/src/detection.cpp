#include <memory>
#include <vector>

#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/imgproc.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <vision_msgs/msg/detection2_d_array.hpp>

// Turns the binary mask from the mask node into Detection2D messages (bounding boxes).
class Detection : public rclcpp::Node
{
public:
  Detection()
  : Node("detection")
  {
    // Read per frame so it can be tuned live.
    declare_parameter<double>("min_area", 500.0);

    detections_pub_ = create_publisher<vision_msgs::msg::Detection2DArray>("/detections", 10);

    subscription_ = create_subscription<sensor_msgs::msg::Image>(
      "/image_mask", rclcpp::SensorDataQoS().reliable(),
      [this](const sensor_msgs::msg::Image::ConstSharedPtr & msg) {onMask(msg);});
  }

private:
  void onMask(const sensor_msgs::msg::Image::ConstSharedPtr & msg)
  {
    cv_bridge::CvImageConstPtr mask;
    try {
      mask = cv_bridge::toCvShare(msg, "mono8");
    } catch (const cv_bridge::Exception & e) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "cv_bridge: %s", e.what());
      return;
    }

    const double min_area = get_parameter("min_area").as_double();

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask->image, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    vision_msgs::msg::Detection2DArray detections;
    detections.header = msg->header;

    for (const auto & contour : contours) {
      if (cv::contourArea(contour) < min_area) {
        continue;
      }
      const cv::Rect box = cv::boundingRect(contour);

      vision_msgs::msg::Detection2D detection;
      detection.header = msg->header;
      detection.bbox.center.position.x = box.x + box.width / 2.0;
      detection.bbox.center.position.y = box.y + box.height / 2.0;
      detection.bbox.size_x = box.width;
      detection.bbox.size_y = box.height;

      vision_msgs::msg::ObjectHypothesisWithPose hypothesis;
      hypothesis.hypothesis.class_id = "blob";
      hypothesis.hypothesis.score = 1.0;
      detection.results.push_back(hypothesis);

      detections.detections.push_back(detection);
    }

    detections_pub_->publish(detections);
  }

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
  rclcpp::Publisher<vision_msgs::msg::Detection2DArray>::SharedPtr detections_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Detection>());
  rclcpp::shutdown();
  return 0;
}
