#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bitset>

#define REGCOUNT = 32
#define MEMSIZE = 997

struct Memory
{
	int address;
	int contents;
};

void initialize(std::ifstream *in, std::ofstream *file, std::vector<Memory> regs, std::vector<Memory> mem)
{
	//Read
	std::string el, data;
	*in >> el;
	if (el == "REGISTERS")
	{
		while (true)
		{
			*in >> el;
			if (el == "MEMORY" || el == "CODE")
			{
				break;
			}
			*in >> data;
			regs.push_back({std::stoi(el.substr(1, el.length())), std::stoi(data)});
		}
	}
	if (el == "MEMORY")
	{
		while (true)
		{
			*in >> el;
			if (el == "CODE")
			{
				break;
			}
			*in >> data;
			mem.push_back({std::stoi(el), std::stoi(data)});
		}
	}
	//Write
	*file << "REGISTERS" << std::endl;
	for (int i = 0; i < regs.size(); i++)
	{
		*file << "R" << regs[i].address << " " << regs[i].contents << std::endl;
	}
	*file << "MEMORY" << std::endl;
	for (int i = 0; i < mem.size(); i++)
	{
		*file << mem[i].address << " " << mem[i].contents << std::endl;
	}
}

std::string regToBinary(std::string reg)
{
	return std::bitset<5>(std::stoi(reg.substr(1, reg.length()))).to_string();
}

std::string immediateToBinary(std::string immediate)
{
	return std::bitset<16>(std::stoi(immediate)).to_string();
}

std::string trim(std::string str)
{
	std::string whitespace = " \t\f\n\r\v";
	return str.substr(str.find_first_not_of(whitespace),str.find_last_not_of(whitespace)+1);
}

int getCurrentLineNumber(std::ifstream* in)
{
	int init = in->tellg();
	int lineNumber = 0;
	std::string line;
	in->seekg(0);
	while (in->tellg() < init)
	{
		std::getline(*in, line);
		lineNumber++;
	}
	in->seekg(init);
	std::cout << "Current line is " << lineNumber << std::endl;
	return lineNumber;
}

int getLabelLineNumber(std::ifstream* in, std::string label)
{
	std::cout << "Looking for label " << label + ":" << std::endl;
	int init = in->tellg();
	int lineNumber = 0;
	std::string line="";
	in->seekg(0);
	while (line.find(label + ":") == std::string::npos)
	{
		std::getline(*in, line);
		lineNumber++;
	}
	in->seekg(init);
	std::cout << "Label found on line number " << lineNumber << std::endl;
	return lineNumber;
}

void ITypeHandler(std::ifstream *in, std::ofstream *out, int code)
{
	if (code == 0)	//LW, SW
	{
		std::string rs, rtImm, rt, imm;
		*in >> rs >> rtImm;
		//Remove commas
		rs = rs.substr(0, rs.length()-1);
		std::cout << " rt+imm " << rtImm << " rs " << rs << std::endl;
		if (rtImm.find('(') != std::string::npos)
		{
			int pos = rtImm.find('(');
			imm = rtImm.substr(0, pos);
			rt = rtImm.substr(pos+1, rtImm.find(')')-pos-1);
			std::cout << " rt " << rt << " imm " << imm << std::endl;
			*out << regToBinary(rt) << regToBinary(rs) << immediateToBinary(imm);
		}
		else
		{
			//immediate address is 0 (is this the only other possibility?) AAA
			*out << regToBinary("R0") << regToBinary(rs) << immediateToBinary(rtImm);
		}
	}
	else //ADDI, BEQ, BNE
	{
		std::string rs, rt, imm;
		*in >> rt >> rs >> imm;
		//Remove commas
		rs = rs.substr(0, rs.length()-1);
		rt = rt.substr(0, rt.length()-1);
		
		if(code == 1)
		{
			*out << regToBinary(rs) << regToBinary(rt) << immediateToBinary(imm);
		}
		else
		{
			*out << regToBinary(rt) << regToBinary(rs);
			int lineDiff = getLabelLineNumber(in, imm) - getCurrentLineNumber(in) - 1;
			*out << immediateToBinary(std::to_string(lineDiff));
		}
	}
	*out << std::endl;
}

void RTypeHandler(std::ifstream* in, std::ofstream* out)
{
	std::string rd, rs, rt;
	*in >> rd >> rs >> rt;
	//Remove commas
	rd = rd.substr(0, rd.length()-1);
	rs = rs.substr(0, rs.length()-1);
	*out << "000000" << regToBinary(rs) << regToBinary(rt) << regToBinary(rd);
}

void compile(std::ifstream *in, std::ofstream *out)
{
	*out << "CODE" << std::endl;
	std::string line;
	//Get line
	while (*in >> line)
	{
		std::cout << line << std::endl;
		if (line == "LW")
		{
			//LW $rt, 32($rs)
			*out << "100011";
			ITypeHandler(in, out, 0);
		}
		else if (line == "SW")
		{
			//SW $rt, 32($rs)
			*out << "101011";
			ITypeHandler(in, out, 0);
		}
		else if (line == "ADD")
		{
			//add $rd, $rs, $rt
			RTypeHandler(in, out);
			*out << "00000";					//SHAMT
			*out << "100000" << std::endl;		//FUNCT
		}
		else if (line == "ADDI")
		{
			//addi $rt, $rs, 4
			*out << "001000";
			ITypeHandler(in, out, 1);
		}
		else if (line == "SUB")
		{
			//sub $rd, $rs, $rt
			RTypeHandler(in, out);
			*out << "00000";					//SHAMT
			*out << "100010" << std::endl;		//FUNCT
		}
		else if (line == "SLT")
		{
			//slt $rd, $rs, $rt
			RTypeHandler(in, out);
			*out << "00000";					//SHAMT
			*out << "101010" << std::endl;		//FUNCT
		}
		else if (line == "BEQ")
		{
			//beq $rt, $rs, Label
			*out << "000100";
			ITypeHandler(in, out, 2);

		}
		else if (line == "BNE")
		{
			//bne $rt, $rs, Label
			*out << "000101";
			ITypeHandler(in, out, 2);
		}
		else
		{
			//Might be a label
		}

	}
}

int main(int argc, char *argv[])
{
	std::vector<Memory> regInit {
		//{1, 16},
		//{3, 42},
		//{5, 8}
		};
	std::vector<Memory> memoryInit {
		//{8, 40},
		//{16, 60}
	};
	std::ofstream out;
	out.open(argv[1]);
	std::ifstream in;
	in.open(argv[2]);
	initialize(&in, &out, regInit, memoryInit);
	compile(&in, &out);
	out.close();
	in.close();
}