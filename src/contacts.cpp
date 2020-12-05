// API for contact storing in the wallet
// Created by Atli Guðjónsson and Sindri Unnsteinsson
// as part of the final project in STÆ532M

// #include <cassert>
// #include "leveldb/include/leveldb/db.h"
//
// #include "contacts.h"
//
//
// // Put method. Store contact in database.
// bool Contacts::save(const std::string& key, const std::string& value) {
  // leveldb::DB* db;
  // leveldb::Options options;
  // options.create_if_missin = true;
  // leveldb::Status s = db->put(leveldb::WriteOptions(), key, value&);
//
  // bool ret_status = false;
//
  // if(s.ok()) {
      // ret_status = true;
  // }
//
  // delete db;
  // db = null;
//
  // return ret_status;
// }
//
//
// // Get method. Find contact in database
// std::string Contacts::get(const std::string& key) {
  // leveldb::DB* db;
  // leveldb::Options options;
  // options.create_if_missin = true;
  // leveldb::Status status = leveldb::DB::Open(options, "~/.smileycoin/contacts", &db);
//
  // assert(status.ok());
//
  // std::string value;
//
  // leveldb::Status s = db->get(leveldb::ReadOptions(), key, value);
//
  // delete db;
  // db = null;
//
  // return value
// }
//
