#include <cstring>
#include <memory>
#include <string>

#include <librealsense2/rs.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

class Image : public rclcpp::Node
{
public:
  Image()
  : Node("image")
  {
    width_ = declare_parameter<int>("width", 1920);
    height_ = declare_parameter<int>("height", 1080);
    fps_ = declare_parameter<int>("fps", 30);
    frame_id_ = declare_parameter<std::string>("frame_id", "camera_color_optical_frame");
    const auto serial = declare_parameter<std::string>("serial_no", "");

    publisher_ = create_publisher<sensor_msgs::msg::Image>(
      "/image_raw", rclcpp::SensorDataQoS().reliable());

    rs2::config cfg;
    if (!serial.empty()) {
      cfg.enable_device(serial);
    }
    cfg.enable_stream(RS2_STREAM_COLOR, width_, height_, RS2_FORMAT_BGR8, fps_);

    // The SDK calls onFrame on its own thread each time a frame arrives.
    auto profile = pipeline_.start(cfg, [this](const rs2::frame & frame) {onFrame(frame);});
    auto device = profile.get_device();

    RCLCPP_INFO(
      get_logger(), "Started %s (S/N %s) color stream %dx%d @ %d fps",
      device.get_info(RS2_CAMERA_INFO_NAME),
      device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER),
      width_, height_, fps_);
  }

  ~Image() override
  {
    // Stops the SDK callback thread before the publisher is destroyed.
    pipeline_.stop();
  }

private:
  void onFrame(const rs2::frame & frame)
  {
    rs2::video_frame color = frame;
    if (auto frames = frame.as<rs2::frameset>()) {
      color = frames.get_color_frame();
    }
    if (!color) {
      return;
    }

    auto msg = std::make_unique<sensor_msgs::msg::Image>();
    msg->header.stamp = now();
    msg->header.frame_id = frame_id_;
    msg->width = color.get_width();
    msg->height = color.get_height();
    msg->encoding = "bgr8";
    msg->is_bigendian = false;
    msg->step = color.get_stride_in_bytes();
    msg->data.resize(static_cast<size_t>(msg->step) * msg->height);
    std::memcpy(msg->data.data(), color.get_data(), msg->data.size());

    publisher_->publish(std::move(msg));
  }

  rs2::pipeline pipeline_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;

  int width_;
  int height_;
  int fps_;

  std::string frame_id_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<Image>());
  } catch (const rs2::error & e) {
    RCLCPP_FATAL(
      rclcpp::get_logger("Image"), "RealSense error calling %s(%s): %s",
      e.get_failed_function().c_str(), e.get_failed_args().c_str(), e.what());
  }
  rclcpp::shutdown();
  return 0;
}
