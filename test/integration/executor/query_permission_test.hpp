/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EXECUTOR_QUERY_PERMISSION_TEST_HPP
#define EXECUTOR_QUERY_PERMISSION_TEST_HPP

#include "framework/common_constants.hpp"
#include "framework/executor_itf/executor_itf.hpp"
#include "framework/result_gtest_checkers.hpp"
#include "integration/executor/executor_fixture.hpp"
#include "integration/executor/executor_fixture_param_provider.hpp"

namespace executor_testing {
  namespace query_permission_test {

    struct SpecificQueryPermissionTestData {
      shared_model::interface::RolePermissionSet spectator_permissions;
      shared_model::interface::types::AccountIdType spectator;
      bool enough_permissions;
      std::string description;
    };

    decltype(::testing::Combine(
        executor_testing::getExecutorTestParams(),
        ::testing::ValuesIn({SpecificQueryPermissionTestData{}})))
    getParams(
        shared_model::interface::RolePermissionSet permission_to_query_myself,
        shared_model::interface::RolePermissionSet
            permission_to_query_my_domain,
        shared_model::interface::RolePermissionSet
            permission_to_query_everyone);

    template <typename SpecificQueryFixture>
    struct QueryPermissionTest
        : public SpecificQueryFixture,
          public ::testing::WithParamInterface<
              std::tuple<std::shared_ptr<ExecutorTestParam>,
                         SpecificQueryPermissionTestData>> {
      QueryPermissionTest()
          : backend_param_(std::get<0>(GetParam())),
            permissions_param_(std::get<1>(GetParam())) {}

      iroha::integration_framework::ExecutorItf &getItf() {
        return SpecificQueryFixture::getItf();
      }

      /**
       * Prepare state of ledger:
       * - create accounts of target user, close and remote spectators. Close
       *   spectator is another user from the same domain as the domain of
       * target user account, remote - a user from domain different to domain
       * of target user account.
       *
       * @param target_permissions - set of permissions for target user
       */
      void prepareState(
          shared_model::interface::RolePermissionSet target_permissions) {
        using namespace common_constants;
        using namespace framework::expected;
        // create target user
        target_permissions |= permissions_param_.spectator_permissions;
        assertResultValue(getItf().createUserWithPerms(
            kUser, kDomain, kUserKeypair.publicKey(), target_permissions));
        // create spectators
        assertResultValue(getItf().createUserWithPerms(
            kSecondUser,
            kDomain,
            kSameDomainUserKeypair.publicKey(),
            permissions_param_.spectator_permissions));
        assertResultValue(getItf().createUserWithPerms(
            kSecondUser,
            kSecondDomain,
            kSecondDomainUserKeypair.publicKey(),
            permissions_param_.spectator_permissions));
      }

      const shared_model::interface::types::AccountIdType &getSpectator()
          const {
        return permissions_param_.spectator;
      }

      /// Check a response.
      template <typename SpecificQueryResponse,
                typename SpecificQueryResponseChecker>
      void checkResponse(const iroha::ametsuchi::QueryExecutorResult &response,
                         SpecificQueryResponseChecker checker) {
        if (permissions_param_.enough_permissions) {
          checkSuccessfulResult<SpecificQueryResponse>(response, checker);
        } else {
          checkQueryError<shared_model::interface::StatefulFailedErrorResponse>(
              response, error_codes::kNoPermissions);
        }
      }

     protected:
      virtual std::shared_ptr<ExecutorTestParam> getBackendParam() {
        return backend_param_;
      }

     private:
      const std::shared_ptr<ExecutorTestParam> &backend_param_;
      const SpecificQueryPermissionTestData &permissions_param_;
    };

    std::string paramToString(
        testing::TestParamInfo<std::tuple<std::shared_ptr<ExecutorTestParam>,
                                          SpecificQueryPermissionTestData>>
            param);

  }  // namespace query_permission_test
}  // namespace executor_testing

#endif /* EXECUTOR_QUERY_PERMISSION_TEST_HPP */
