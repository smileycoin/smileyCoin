2.2.5 Release notes
===================

- Fix build error due to change in OpenSSL 1.1 API
- Add build instructions for Fedora
- Add an option to reqeuest a vanity address
- Add `datacarriersize` option to allow the wallet to 
  accept an arbitrarily large `OP_RETURN`
- Add `datacarriersize` option to allow the wallet to accept an arbitrarily
  large `OP_RETURN`
- Add a `sequence` field to `createrawtransaction`, allowing the user to set
  an arbitrary `nSequence` to the transaction.
- Rename `OP_NOP2` to `OP_CHECKLOCKTIMEVERIFY` with fallbacks to preserve 
  compatibility
- Building from source now requires a C++11 compatible compiler.
- Fix build error due to API change in Boost versions > 1.69
- Changed AreInputsStandard to allow for more varied P2SH transactions
  (see https://gist.github.com/gavinandresen/88be40c141bc67acb247 and
   https://github.com/bitcoin/bitcoin/pull/4365/commits/7f3b4e95695d50a4970e6eb91faa956ab276f161)
