// Copyright 2019 Fraunhofer IPA
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ACTION_BRIDGE__ACTION_BRIDGE_HPP_
#define ACTION_BRIDGE__ACTION_BRIDGE_HPP_

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunused-parameter"
#endif
#include <ros/ros.h>
#include <actionlib/server/action_server.h>
#include <actionlib/client/action_client.h>
#include <actionlib/client/simple_action_client.h>
#include "ros/callback_queue.h"
#include <actionlib_tutorials/FibonacciAction.h>

#ifdef __clang__
# pragma clang diagnostic pop
#endif

#define BOOST_BIND_NO_PLACEHOLDERS

// include ROS 2
#include "rclcpp/rclcpp.hpp"
#include <rclcpp_action/rclcpp_action.hpp>
#include <action_tutorials/action/fibonacci.hpp>

//#include <boost/scoped_ptr.hpp>

#include <algorithm>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

//template<class ROS1_T, class ROS2_T>
class ActionBridge
{
public:
  using ROS1_T = actionlib_tutorials::FibonacciAction;
  using ROS2_T = action_tutorials::action::Fibonacci;

  using ROS2GoalHandle = typename rclcpp_action::ServerGoalHandle<ROS2_T>;


  using ROS1Goal = typename actionlib::ActionServer<ROS1_T>::Goal;
  using ROS1Feedback = typename actionlib::ActionServer<ROS1_T>::Feedback;
  using ROS1Result = typename actionlib::ActionServer<ROS1_T>::Result;

  using ROS2Goal = typename ROS2_T::Goal;
  using ROS2Feedback = typename ROS2_T::Feedback;
  using ROS2Result = typename ROS2_T::Result;

//  using ROS1GoalHandle = typename actionlib::ActionClient<ROS1_T>::GoalHandle;
//  using ROS1Client = typename actionlib::ActionClient<ROS1_T>;

//  using ROS2SendGoalOptions = typename rclcpp_action::Client<ROS2_T>::SendGoalOptions;
//  using ROS2ServerSharedPtr = typename rclcpp_action::Server<ROS2_T>::SharedPtr;

  ActionBridge(ros::NodeHandle ros1_node,
               rclcpp::Node::SharedPtr ros2_node,
               const std::string action_name)
    : ros1_node_(ros1_node)
    , ros2_node_(ros2_node)
    , client_(action_name, true)
  {
//    client_.reset(new actionlib::ActionClient<ROS1_T>(ros1_node_, action_name, &callback_queue)); // ROS1 action client

    server_ = rclcpp_action::create_server<ROS2_T>(ros2_node_->get_node_base_interface(),
                                                   ros2_node_->get_node_clock_interface(),
                                                   ros2_node_->get_node_logging_interface(),
                                                   ros2_node_->get_node_waitables_interface(),
                                                   action_name,
                                                   std::bind(&ActionBridge::handle_goal, this, std::placeholders::_1, std::placeholders::_2),
                                                   std::bind(&ActionBridge::handle_cancel, this, std::placeholders::_1),
                                                   std::bind(&ActionBridge::handle_accepted, this, std::placeholders::_1)
                                                   );

    std::cout << "Looking for a ROS1 action server named " << action_name << std::endl;
    if (!client_.waitForServer(ros::Duration(1,0)))
    {
      std::cout << "ROS1 action server not started yet" << std::endl;
    }
  }

  rclcpp_action::GoalResponse handle_goal(
      const rclcpp_action::GoalUUID & uuid,
      std::shared_ptr<const ROS2Goal> goal)
  {
    (void)uuid;
//    TODO: if ROS1 server rejects the goal, we should reject the goal here
//      if (false) {
//        return rclcpp_action::GoalResponse::REJECT;
//      }
      return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(
      const std::shared_ptr<ROS2GoalHandle> goal_handle)
  {
    // TODO: Pass along cancellation to ROS1 server
      (void)goal_handle;
      client_.cancelGoal();
      return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<ROS2GoalHandle> goal_handle)
  {
    std::thread{std::bind(&ActionBridge::goal_execute, this, std::placeholders::_1), goal_handle}.detach();
  }

  void goal_execute(const std::shared_ptr<ROS2GoalHandle> goal_handle)
  {
    std::cout << "Executing goal" << std::endl;
    ROS1Goal goal1;
    translate_goal_2_to_1(*goal_handle.get_goal(), goal1);

    client_.sendGoal(goal1, );
    // TODO: make ROS1 server execute the goal
  }

  void handle_done(const actionlib::SimpleClientGoalState& state,
                   const ROS1Result::ConstPtr)
  {

  }

  void handle_active()
  {

  }

  void handle_feedback(const ROS1Result::ConstPtr)
  {

  }




//TO DO: check Goal Handling for ROS 2
//  void cancel_cb(ROS2GoalHandle gh2)
//  {
//    // try to find goal and cancel it
//    std::lock_guard<std::mutex> lock(mutex_);
//    auto it = goals_.find(gh2.getGoalID().id);
//    if (it != goals_.end()) {
//      std::thread([handler = it->second]() mutable {
//          handler->cancel();
//        }).detach();
//    }
//  }

//  void goal_cb(ROS2GoalHandle gh2)
//  {
//    const std::string goal_id = gh2.getGoalID().id;

//    // create a new handler for the goal
//    std::shared_ptr<GoalHandler> handler;
//    handler.reset(new GoalHandler(gh2, client_));
//    std::lock_guard<std::mutex> lock(mutex_);
//    goals_.insert(std::make_pair(goal_id, handler));

//    //RCLCPP_INFO(ros1_node.sendGoal(), "Sending goal");
//    std::thread([handler, goal_id, this]() mutable {
//        // execute the goal remotely
//        handler->handle();

//        // clean-up
//        std::lock_guard<std::mutex> lock(mutex_);
//        goals_.erase(goal_id);
//      }).detach();
//  }

//  void accepted_cb(ROS2GoalHandle gh2)
//  {
//    return;
//  }

  static int main(const std::string & action_name, int argc, char * argv[])
  {
    std::string node_name = "reverse_action_bridge_" + action_name;
    std::replace(node_name.begin(), node_name.end(), '/', '_');
    // ROS 1 node
    ros::init(argc, argv, node_name);
    ros::NodeHandle ros1_node;

    // ROS 2 node
    rclcpp::init(argc, argv);
    auto ros2_node = rclcpp::Node::make_shared(node_name);

//    ActionBridge<ROS1_T, ROS2_T> action_bridge(ros1_node, ros2_node, action_name);
    ActionBridge action_bridge(ros1_node, ros2_node, action_name);


    // // ROS 1 asynchronous spinner
    ros::AsyncSpinner async_spinner(0);
    async_spinner.start();
    rclcpp::spin(ros2_node);
    ros::shutdown();
    return 0;
  }

private:
//  using ROS1Goal = typename actionlib::ActionServer<ROS1_T>::Goal;
//  using ROS1Feedback = typename actionlib::ActionServer<ROS1_T>::Feedback;
//  using ROS1Result = typename actionlib::ActionServer<ROS1_T>::Result;
//  using ROS2Goal = typename ROS2_T::Goal;
//  using ROS2Feedback = typename ROS2_T::Feedback;
//  using ROS2Result = typename ROS2_T::Result;
//  using ROS2GoalHandle = typename rclcpp_action::ClientGoalHandle<ROS2_T>::SharedPtr;
//  using ROS2ClientSharedPtr = typename rclcpp_action::Client<ROS2_T>::SharedPtr;
//  using ROS2SendGoalOptions = typename rclcpp_action::Client<ROS2_T>::SendGoalOptions;



//  class GoalHandler
//  {
//  public:
//    GoalHandler(ROS2GoalHandle & gh1, ROS1Client & client)
//    : gh1_(gh1)
//    , gh2_(nullptr)
//    , client_(client)
//    , canceled_(false)
//    {

//    }

//    void cancel()
//    {
//      std::lock_guard<std::mutex> lock(mutex_);
//      canceled_ = true;
//      if (gh1_) { // cancel goal if possible
//        auto fut = client_->async_cancel_goal(gh1_);
//      }
//    }
//    void handle()
//    {
//      auto goal2 = gh2_.get_goal();
//      ROS1Goal goal1;
//      translate_goal_2_to_1(*gh2_.get_goal(), goal2);

//      if (!client_.waitForActionServerToStart(ros::Duration(1,0))) {
//        std::cout << "Action server not available after waiting" << std::endl;
//        gh2_.setRejected();
//        return;
//      }

//      //Changes as per Dashing
//      auto send_goal_ops = ROS2SendGoalOptions();
//      send_goal_ops.goal_response_callback =
//        [this](auto gh2_future) mutable
//      {
//        auto goal_handle = gh2_future.get();
//        if (!goal_handle) {
//          std::cout << "Could not find goal_handle" <<std::endl;
//          gh1_.setRejected(); // goal was not accepted by remote server
//          return;
//        }

//        gh1_.setAccepted();

//        {
//          std::lock_guard<std::mutex> lock(mutex_);
//          gh2_ = goal_handle;

//          if (canceled_) { // cancel was called in between
//            auto fut = client_->async_cancel_goal(gh2_);
//          }
//        }
//      };

//      send_goal_ops.feedback_callback =
//        [this](ROS2GoalHandle, auto feedback2) mutable
//      {
//        ROS1Feedback feedback1;
//        translate_feedback_2_to_1(feedback1, *feedback2);
//        gh1_.publishFeedback(feedback1);
//      };

//      //reverse these
//      // send goal to ROS2 server, set-up feedback
//      auto gh2_future = client_->async_send_goal(goal2, send_goal_ops);

//      auto future_result = client_->async_get_result(gh2_future.get());
//      auto res2 = future_result.get();

//      ROS1Result res1;
//      translate_result_2_to_1(res1, *(res2.result));

//      std::lock_guard<std::mutex> lock(mutex_);
//      if (res2.code == rclcpp_action::ResultCode::SUCCEEDED) {
//        gh1_.setSucceeded(res1);
//      } else if (res2.code == rclcpp_action::ResultCode::CANCELED) {
//        gh1_.setCanceled(res1);
//      } else {
//        gh1_.setAborted(res1);
//      }

//    }

//  private:
//    ROS1GoalHandle gh1_;
//    ROS2GoalHandle gh2_;
//    ROS1Client client_;
//    bool canceled_; // cancel was called
//    std::mutex mutex_;
//  };

  ros::NodeHandle ros1_node_;
  rclcpp::Node::SharedPtr ros2_node_;

  //defining ROS1 server
//  actionlib::ActionClient<ROS1_T> client_;

//  ros::CallbackQueue callback_queue;
//  std::unique_ptr<actionlib::ActionClient<ROS1_T>> client_;
  actionlib::SimpleActionClient<ROS1_T> client_;

  //defining ROS2 client
  ROS2ServerSharedPtr server_;

  std::mutex mutex_;
//  std::map<std::string, std::shared_ptr<GoalHandler>> goals_;

  static void translate_goal_1_to_2(const ROS1Goal &, ROS2Goal &);
  static void translate_result_2_to_1(ROS1Result &, const ROS2Result &);
  static void translate_feedback_2_to_1(ROS1Feedback &, const ROS2Feedback &);

  static void translate_goal_2_to_1(const ROS2Goal &, ROS1Goal &);
  static void translate_result_1_to_2(ROS2Result &, const ROS1Result &);
  static void translate_feedback_1_to_2(ROS2Feedback &, const ROS1Feedback &);
};



#endif // ACTION_BRIDGE__ACTION_BRIDGE_HPP_

