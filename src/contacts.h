// API for contact storing in the wallet
// Created by Atli Guðjónsson and Sindri Unnsteinsson
// as part of the final project in STÆ532M
#ifndef CONTACTS_LEVELDB_H
#define CONTACTS_LEVELDB_H

#include <string>
#include "leveldb/db.h"

template<typename K, typename V> bool save(const K& key, const V& value) {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, "/home/atli/.smileycoin/contacts", &db);

  assert(status.ok());

  leveldb::Status s = db->Put(leveldb::WriteOptions(), key, value);

  bool ret_status = false;

  if(s.ok()) {
      ret_status = true;
  }

  delete db;
  db = NULL;

  return ret_status;
}

template<typename K> std::string get(const std::string& key) {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, "~/.smileycoin/contacts", &db);

  assert(status.ok());

  std::string value;

  leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);

  delete db;
  db = NULL;

  return value;
}

#endif
