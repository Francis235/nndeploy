
#include "nndeploy/dag/executor/sequential_executor.h"

namespace nndeploy {
namespace dag {

SequentialExecutor::SequentialExecutor() : Executor(){};
SequentialExecutor::~SequentialExecutor(){};

base::Status SequentialExecutor::init(
    std::vector<EdgeWrapper *> &edge_repository,
    std::vector<NodeWrapper *> &node_repository) {
  base::Status status = topoSortDFS(node_repository, topo_sort_node_);
  for (auto iter : topo_sort_node_) {
    iter->node_->setInitializedFlag(false);
    status = iter->node_->init();
    if (status != base::kStatusCodeOk) {
      NNDEPLOY_LOGE("Node %s init failed\n", iter->node_->getName().c_str());
      return status;
    }
    iter->node_->setInitializedFlag(true);
  }
  return status;
}
base::Status SequentialExecutor::deinit() {
  base::Status status = base::kStatusCodeOk;
  for (auto iter : topo_sort_node_) {
    status = iter->node_->deinit();
    NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk,
                           "failed iter->node_->deinit()");
    iter->node_->setInitializedFlag(false);
  }
  return status;
}

base::Status SequentialExecutor::run() {
  base::Status status = base::kStatusCodeOk;
  for (auto iter : topo_sort_node_) {
    iter->node_->setRunningFlag(true);
    bool terminate_flag = false;
    auto inputs = iter->node_->getAllInput();
    for (auto input : inputs) {
      // NNDEPLOY_LOGE("Node name[%s], Thread ID: %d.\n",
      //               iter->node_->getName().c_str(),
      //               std::this_thread::get_id());
      bool flag = input->update(iter->node_);
      // NNDEPLOY_LOGE("Node name[%s], Thread ID: %d.\n",
      //               iter->node_->getName().c_str(),
      //               std::this_thread::get_id());
      if (!flag) {
        terminate_flag = true;
        break;
      }
    }
    if (terminate_flag) {
      break;
    }
    status = iter->node_->run();
    NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk,
                           "node execute failed!\n");
    iter->node_->setRunningFlag(false);
  }
  return status;
}

}  // namespace dag
}  // namespace nndeploy
