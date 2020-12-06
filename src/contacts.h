// API for storing contacts in the wallet
// Created by Atli Guðjónsson and Sindri Unnsteinsson
// as part of our final project in STÆ532M fall 2020
//
// Methods:
// 
// save(key, value) - stores given key-value pair.
//
// getContac(key) - finds and returns the address associated with the key.
//
// getAll() - returns all contact key-value pairs stored in the wallet.
//
// deleteContact(key) - removes a key-value pair from the storage.

#ifndef CONTACTS_LEVELDB_H
#define CONTACTS_LEVELDB_H

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "leveldb/db.h"

std::string getPath() {
  // Fetch the path to the database
  const char *homedir;
  if ((homedir = getenv("HOME")) == NULL) {
    homedir = getpwuid(getuid())->pw_dir;
  }

  std::string home(homedir);
  std::string data("/.smileycoin/contacts");

  std::string path = home + data;
  return path;
}


template<typename K, typename V> bool save(const K& key, const V& value) {
  std::string path = getPath();

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, path, &db);

  if (!status.ok()) {
    std::string error = status.ToString();
    throw runtime_error(error);
  }

  leveldb::Status s = db->Put(leveldb::WriteOptions(), key, value);

  bool ret_status = false;

  if(s.ok()) {
      ret_status = true;
  }

  delete db;
  db = NULL;

  return ret_status;
}

template<typename K> std::string getContact(const K& key) {
  std::string path = getPath();

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, path, &db);

  if (!status.ok()) {
    std::string error = status.ToString();
    throw runtime_error(error);
  }
 
  std::string value;

  leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);

  delete db;
  db = NULL;

  return value;
}

std::string getAll() {
  std::string path = getPath();

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, path, &db);

  if (!status.ok()) {
    std::string error = status.ToString();
    throw runtime_error(error);
  }

  std::string output = "Contacts:\n";
  leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());

  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    output = output + it->key().ToString() + ": " + it->value().ToString() + "\n";
  }

  assert(it->status().ok());  // Check for any errors found during the scan
  delete it;
  it = NULL;
  delete db;
  db = NULL;
  
  return output;
}

template<typename K> bool deleteContact(const K &key) {
  std::string path = getPath();

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, path, &db);

  if (!status.ok()) {
    std::string error = status.ToString();
    throw runtime_error(error);
  }

  leveldb::Status s = db->Delete(leveldb::WriteOptions(), key);
 
  bool ret_status = false;

  if(s.ok()) {
      ret_status = true;
  }

  delete db;
  db = NULL;

  return ret_status; 
}

#endif
