#include <algorithm>
#include <memory>
#include <vector>

#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/imgproc.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

class Mask : public rclcpp::Node
{
public:
  Mask()
  : Node("mask")
  {
    declare_parameter<int>("h_low", 170);
    declare_parameter<int>("s_low", 200);
    declare_parameter<int>("v_low", 8);

    declare_parameter<int>("h_high", 10);
    declare_parameter<int>("s_high", 255);
    declare_parameter<int>("v_high", 255);

    declare_parameter<int>("kernel_size", 5);
    declare_parameter<double>("min_area", 500.0);

    mask_pub_ = create_publisher<sensor_msgs::msg::Image>(
      "/image_mask", rclcpp::SensorDataQoS().reliable());

    contour_pub_ = create_publisher<sensor_msgs::msg::Image>(
      "/image_contour", rclcpp::SensorDataQoS().reliable());

    subscription_ = create_subscription<sensor_msgs::msg::Image>(
      "/image_raw", rclcpp::SensorDataQoS().reliable(),
      [this](const sensor_msgs::msg::Image::ConstSharedPtr & msg) { onImage(msg); });
  }

private:
  void onImage(const sensor_msgs::msg::Image::ConstSharedPtr & msg)
  {
    cv_bridge::CvImageConstPtr src;

    try {
      src = cv_bridge::toCvShare(msg, "bgr8");
    } catch (const cv_bridge::Exception & e) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "cv_bridge: %s", e.what());
      return;
    }

    const cv::Scalar low(
      get_parameter("h_low").as_int(),
      get_parameter("s_low").as_int(),
      get_parameter("v_low").as_int());
    
    const cv::Scalar high(
      get_parameter("h_high").as_int(),
      get_parameter("s_high").as_int(),
      get_parameter("v_high").as_int());
    
      const int k = std::max(1, static_cast<int>(get_parameter("kernel_size").as_int()));
    const double min_area = get_parameter("min_area").as_double();

    // HSV threshold -> binary mask.
    cv::Mat hsv, mask;
    cv::cvtColor(src->image, hsv, cv::COLOR_BGR2HSV);
    if (low[0] <= high[0]) {
      cv::inRange(hsv, low, high, mask);
    } else {
      // OpenCV hue is 0-179; split the wrapped range into [h_low, 179] and [0, h_high].
      cv::Mat upper;
      cv::inRange(hsv, low, cv::Scalar(179, high[1], high[2]), mask);
      cv::inRange(hsv, cv::Scalar(0, low[1], low[2]), high, upper);
      mask |= upper;
    }

    // Open removes small noise, close fills small holes.
    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(k, k));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat overlay = src->image.clone();
    for (const auto & contour : contours) {
      if (cv::contourArea(contour) < min_area) {
        continue;
      }
      cv::drawContours(overlay, std::vector<std::vector<cv::Point>>{contour}, -1,
        cv::Scalar(0, 255, 0), 2);
      cv::rectangle(overlay, cv::boundingRect(contour), cv::Scalar(255, 0, 0), 2);
      const cv::Moments m = cv::moments(contour);
      if (m.m00 > 0.0) {
        cv::circle(overlay, cv::Point(m.m10 / m.m00, m.m01 / m.m00), 5,
          cv::Scalar(0, 0, 255), cv::FILLED);
      }
    }

    mask_pub_->publish(*cv_bridge::CvImage(msg->header, "mono8", mask).toImageMsg());
    contour_pub_->publish(*cv_bridge::CvImage(msg->header, "bgr8", overlay).toImageMsg());
  }

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr mask_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr contour_pub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Mask>());
  rclcpp::shutdown();
  return 0;
}
