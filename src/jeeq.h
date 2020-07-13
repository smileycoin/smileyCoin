namespace Jeeq 
{
int Init();
int Cleanup();
std::vector<uint8_t> EncryptMessage(CPubKey pubkey, std::vector<uint8_t> msg);
std::vector<uint8_t> DecryptMessage(CKey privkey, std::vector<uint8_t> enc);
}


