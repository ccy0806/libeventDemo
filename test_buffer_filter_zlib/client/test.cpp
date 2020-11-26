#include <iostream>
using namespace std;
int main()
{
	FILE *fp = fopen("file1.txt", "rb");
	char data[1024] = {0};
	int len = fread(data,1, sizeof(data), fp);
	cout << len << endl;
	cout << data <<endl;
	len = fread(data,1, sizeof(data), fp);
	cout << len << endl;
	cout <<data<<endl;
}
