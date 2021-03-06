/**
 *
 *  Copyright 2021 Netflix, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#pragma once

#include "utils/status.h"

#include <string>

#include <aws/s3/S3Client.h>
#include <aws/sqs/SQSClient.h>

namespace memcachedumper {

class AwsUtils {
 public:
  static void SetS3Bucket(std::string s3_bucket);
  static void SetS3Path(std::string s3_path);
  static void SetSQSQueueName(std::string sqs_queue);
  static void SetCachedSQSQueueURL(std::string sqs_url);
  static void SetS3Client(Aws::S3::S3Client* s3_client);
  static void SetSQSClient(Aws::SQS::SQSClient* sqs_client);

  static std::string GetS3Bucket() { return AwsUtils::s3_bucket_; }
  static std::string GetS3Path() { return AwsUtils::s3_path_; }
  static std::string GetSQSQueueName() { return AwsUtils::sqs_queue_name_; }
  static std::string GetCachedSQSQueueURL() { return AwsUtils::sqs_url_; }
  static Aws::S3::S3Client* GetS3Client() { return AwsUtils::s3_client_; }
  static Aws::SQS::SQSClient* GetSQSClient() { return AwsUtils::sqs_client_; }

  static Status GetSQSUrlFromName(std::string& queue_name, std::string* out_url);
  static Status CreateNewSQSQueue(std::string& queue_name, std::string* out_url);

  // Returns a JSON string in 'out_sqs_body' with the following format:
  // {"reqId":'123',"host":'127.0.0.1',"uri":'s3://bucket/file',"keysCount":0,"dumpFormat":'BINARY'}
  static Status SQSBodyForS3(std::string& s3_file_uri, std::string* out_sqs_body);

 private:
  static std::string s3_bucket_;
  static std::string s3_path_;
  static std::string sqs_queue_name_;
  static std::string sqs_url_;
  static Aws::S3::S3Client* s3_client_;
  static Aws::SQS::SQSClient* sqs_client_;
};

} // namespace memcachedumper
