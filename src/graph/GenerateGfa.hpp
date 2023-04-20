
#ifndef MDBG_METAG_GENERATEGFA
#define MDBG_METAG_GENERATEGFA

#include "../Commons.hpp"


class GenerateGfa : public Tool{
    
public:


	string _outputDir;
	size_t _k;
	int _nbCores;

	string _filename_exe;
	string _tmpDir;

	GenerateGfa(): Tool (){

	}


	void parseArgs(int argc, char* argv[]){


		_filename_exe = argv[0];

		args::ArgumentParser parser("gfa", ""); //"This is a test program.", "This goes after the options."
		//args::Positional<std::string> arg_contigs(parser, "contigs", "Contig file generated by metaMDBG", args::Options::Required);
		args::Positional<std::string> arg_outputDir(parser, "assemblyDir", "Output dir of a metaMDBG run", args::Options::Required);
		args::Positional<int> arg_k(parser, "k", "Generate assembly graph for this k-min-mer size value (set to 0 to display available k values and corresponding length in bps) ", args::Options::Required);
		args::ValueFlag<int> arg_nbCores(parser, "", "Number of cores", {ARG_NB_CORES2}, NB_CORES_DEFAULT_INT);
		args::Flag arg_help(parser, "", "", {'h', "help"}, args::Options::Hidden);


		try
		{
			parser.ParseCLI(argc, argv);
		}
		catch (const args::Help&)
		{
			cerr << parser;
			exit(0);
		}
		catch (const std::exception& e)
		{
			cerr << parser;
			//_logFile << endl;
			cerr << e.what() << endl;
			exit(0);
		}

		if(arg_help){
			cerr << parser;
			exit(0);
		}



		_outputDir = args::get(arg_outputDir);
		_k = args::get(arg_k);
		_nbCores = args::get(arg_nbCores);

		if(!fs::exists(_outputDir)){
			cerr << "Assembly dir does not exists" << endl;
			exit(1);
		}

		_tmpDir = _outputDir + "/tmp/";

		if(!fs::exists(_tmpDir)){
			cerr << "Invalid assembly dir" << endl;
			exit(1);
		}

		openLogFile(_tmpDir);

		_logFile << "Assembly dir: " << _outputDir << endl;
		_logFile << "Used k: " << _k << endl;

	}


    void execute (){

		if(_k <= 0){
			displayAvailableK();
		}
		else{

			vector<size_t> kvalues = getAvailableKValues();
			if(std::find(kvalues.begin(), kvalues.end(), _k) == kvalues.end()){
				cerr << "Invalid k value" << endl;
				displayAvailableK();
				exit(1);
			}

			generateAssemblyGraph();
			
			cerr << endl;
			cerr << "Assembly graph filename: " << _finalFilename << endl;
			cerr << "Done!" << endl;

		}

		closeLogFile();


	}

	void displayAvailableK(){

		vector<size_t> kvalues = getAvailableKValues();

		cerr << endl << "Available k-min-mer size and corresponding length in bps:" << endl;

		for(size_t k : kvalues){

			string passDir = _tmpDir + "/pass_k" + to_string(k) + "/";

			size_t minimizerSize;
			size_t kminmerSize;
			float minimizerDensity;

			string filename_parameters = passDir + "/parameters.gz";
			gzFile file_parameters = gzopen(filename_parameters.c_str(),"rb");
			gzread(file_parameters, (char*)&minimizerSize, sizeof(minimizerSize));
			gzread(file_parameters, (char*)&kminmerSize, sizeof(kminmerSize));
			gzread(file_parameters, (char*)&minimizerDensity, sizeof(minimizerDensity));
			gzclose(file_parameters);
			
			cerr << "\t- " << k << " (" << kToKmerSize(kminmerSize, minimizerSize, minimizerDensity) << " bps)" << endl;

		}
	}

	size_t kToKmerSize(size_t kminmerSize, size_t minimizerSize, float minimizerDensity){
		return 1/minimizerDensity * (kminmerSize-1) + minimizerSize;
	}

	vector<size_t> getAvailableKValues(){

		const string pattern = "pass_k";
		vector<size_t> kvalues;

		std::string path = _tmpDir + "/";
		for (const auto & entry : fs::directory_iterator(path)){
			if (!entry.is_directory()) continue;

			string path = entry.path();
			string filename = entry.path().filename();

			if (filename.find(pattern) == string::npos) continue;

			//cout << path << " " << filename << endl;

			filename.erase(0, pattern.size());
			size_t k = stoull(filename);

			//cout << path << " " << k << endl;
			kvalues.push_back(k);
		}

		std::sort(kvalues.begin(), kvalues.end());

		return kvalues;
	}

	string _passDir;
	string _unitigFilename;
	size_t _minimizerSize;
	size_t _kminmerSize;
	float _minimizerDensity;
	string _finalFilename;
	string _finalFilenameNoSeq;

	void generateAssemblyGraph(){


		_passDir = _tmpDir + "/pass_k" + to_string(_k) + "/";
		_unitigFilename = _passDir + "/unitigs.fasta.gz";

		string filename_parameters = _passDir + "/parameters.gz";
		gzFile file_parameters = gzopen(filename_parameters.c_str(),"rb");
		gzread(file_parameters, (char*)&_minimizerSize, sizeof(_minimizerSize));
		gzread(file_parameters, (char*)&_kminmerSize, sizeof(_kminmerSize));
		gzread(file_parameters, (char*)&_minimizerDensity, sizeof(_minimizerDensity));
		//gzread(file_parameters, (char*)&_kminmerSizeFirst, sizeof(_kminmerSizeFirst));
		//gzread(file_parameters, (char*)&_minimizerSpacingMean, sizeof(_minimizerSpacingMean));
		//gzread(file_parameters, (char*)&_kminmerLengthMean, sizeof(_kminmerLengthMean));
		//gzread(file_parameters, (char*)&_kminmerOverlapMean, sizeof(_kminmerOverlapMean));
		//gzread(file_parameters, (char*)&_kminmerSizePrev, sizeof(_kminmerSizePrev));
		gzclose(file_parameters);

		_finalFilename = _outputDir + "/assemblyGraph_k" + to_string(_kminmerSize) + "_" + to_string(kToKmerSize(_kminmerSize, _minimizerSize, _minimizerDensity)) + "bps.gfa";
		_finalFilenameNoSeq = _outputDir + "/assemblyGraph_k" + to_string(_kminmerSize) + "_" + to_string(kToKmerSize(_kminmerSize, _minimizerSize, _minimizerDensity)) + "bps.noseq.gfa";
		
		cerr << "Generating assembly graph for k value: " << _k << " (corresponding sequence length = " << kToKmerSize(_kminmerSize, _minimizerSize, _minimizerDensity) << " bps)" << endl;

		cerr << "Generating unitig sequences..." << endl;
		createUnitigSequences();

		cerr << "Loading unitig sequences..." << endl;
		loadUnitigs();
		//loadUnitigIndex();
		
		cerr << "Creating assembly graph file..." << endl;
		createAssemblyGraphFile();

	}


	//vector<u_int32_t> _unitigIndexes;
	struct UnitigSequence{
		DnaBitset* _seqBitset;
		u_int32_t _overlapSizePlus;
		u_int32_t _overlapSizeMinus;
	};
	vector<UnitigSequence> _unitigSequences;
	phmap::parallel_flat_hash_map<u_int32_t, u_int32_t> _unitigOrder;

	void createUnitigSequences(){
		string readFilename = _tmpDir + "/input.txt";
		string command = _filename_exe + " toBasespace " + " " + _tmpDir + " " + _passDir + "/assembly_graph.gfa.unitigs " + " " + _unitigFilename + " " + readFilename  + " -t " + to_string(_nbCores);
		Utils::executeCommand(command, _tmpDir, _logFile);
	}

	/*
	void loadUnitigIndex(){

		ifstream file_readData(_passDir + "/assembly_graph.gfa.unitigs.index");

		while(true){
			
			u_int32_t unitigIndex;	
			file_readData.read((char*)&unitigIndex, sizeof(unitigIndex));

			if(file_readData.eof()) break;

			_unitigIndexes.push_back(unitigIndex);
			//cout << unitigIndex << endl;
		}

		file_readData.close();		

	}
	*/


	void loadUnitigs(){
		

		ReadParserParallel readParser(_unitigFilename, true, false, 1, _logFile);
		readParser.parse(LoadUnitigsFunctor(*this));

	}

	class LoadUnitigsFunctor {

		public:

		GenerateGfa& _parent;
		MinimizerParser* _minimizerParser;
		EncoderRLE _encoderRLE;

		LoadUnitigsFunctor(GenerateGfa& parent) : _parent(parent){
			_minimizerParser = new MinimizerParser(_parent._minimizerSize, _parent._minimizerDensity);
			_minimizerParser->_trimBps = 0;
		}

		LoadUnitigsFunctor(const LoadUnitigsFunctor& copy) : _parent(copy._parent){
			_minimizerParser = new MinimizerParser(_parent._minimizerSize, _parent._minimizerDensity);
			_minimizerParser->_trimBps = 0;
		}

		~LoadUnitigsFunctor(){
			delete _minimizerParser;
		}


		void operator () (const Read& read) {

			u_int64_t readIndex = read._index;

			string rleSequence;
			vector<u_int64_t> rlePositions;
			_encoderRLE.execute(read._seq.c_str(), read._seq.size(), rleSequence, rlePositions);


			vector<u_int64_t> minimizers;
			vector<u_int64_t> minimizers_pos;
			_minimizerParser->parse(rleSequence, minimizers, minimizers_pos);


			//rlePositions[pos]; i<rlePositions[pos+_readSelection._minimizerSize]

			//cout << read._seq << " " << read._seq.size() << endl;
			
			//for(size_t i=0; i<minimizers.size(); i++){
			//	cout << i << ": " << rlePositions[minimizers_pos[i]] << endl;
			//}

			u_int32_t overlapSizePlus = read._seq.size() - rlePositions[minimizers_pos[minimizers.size()-_parent._kminmerSize+1]];
			u_int32_t overlapSizeMinus = rlePositions[minimizers_pos[_parent._kminmerSize-2]+_parent._minimizerSize];

			//cout << overlapSizeMinus << " " << overlapSizePlus << endl;
			//cout << readIndex << " " << minimizers.size() << endl;
			_parent._unitigSequences.push_back({new DnaBitset(read._seq), overlapSizePlus, overlapSizeMinus});

		}
		
	};

	void createAssemblyGraphFile(){

		string outputFilename = _passDir + "/assembly_graph.gfa.tmp";
		string outputFilenameNoSeq = _passDir + "/assembly_graph.noseq.gfa.tmp";
		ofstream outputFile(outputFilename);
		ofstream outputFileNoSeq(outputFilenameNoSeq);

		u_int64_t unitgIndex = 0;
        ifstream infile(_passDir + "/assembly_graph.gfa");

        std::string line;
        vector<string>* fields = new vector<string>();
        vector<string>* fields_optional = new vector<string>();


        while (std::getline(infile, line)){
            
            GfaParser::tokenize(line, fields, '\t');
            
            //cout << (*fields)[0] << endl;



			if((*fields)[0] == "S"){
				//cout << line << endl;
				//for(size_t i=0; i<(*fields).size(); i++){
				//	cout << i << ": " << (*fields)[i] << endl;
				//}
				//getchar();
				//cout << line << endl;
				//cout << _unitigSequences.size() << " " << unitgIndex << endl;
				DnaBitset* unitigBitset = _unitigSequences[unitgIndex]._seqBitset;
				char* unitigSequence = unitigBitset->to_string();
				string unitigSequenceStr = string(unitigSequence);


				string line = (*fields)[0] + "\t" + (*fields)[1] + "\t" + unitigSequenceStr + "\tLN:i:" + to_string(unitigSequenceStr.size()) + "\t" + (*fields)[4];
				string lineNoSeq = (*fields)[0] + "\t" + (*fields)[1] + "\t" + "*" + "\tLN:i:" + to_string(unitigSequenceStr.size()) + "\t" + (*fields)[4];
				//cout << line << endl;

				outputFile << line << endl;
				outputFileNoSeq << lineNoSeq << endl;

				free(unitigSequence);
				//delete unitigBitset;

				string name = (*fields)[1];
				size_t pos = name.find("utg");
				name.erase(pos, 3);
				u_int32_t utg = stoull(name);

				_unitigOrder[utg] = unitgIndex;

				unitgIndex += 1;
			}

            
        }

        infile.clear();
        infile.seekg(0, std::ios::beg);

        while (std::getline(infile, line)){
            
            GfaParser::tokenize(line, fields, '\t');
            
			if((*fields)[0] == "L"){

				//cout << line << endl;

				string nameFrom = (*fields)[1];
				nameFrom.erase(0, 3);
				u_int32_t from = stoull(nameFrom);

				bool fromOrient = (*fields)[2] == "+";

				string nameTo = (*fields)[3];
				nameTo.erase(0, 3);
				u_int32_t to = stoull(nameTo);

				bool toOrient = (*fields)[4] == "+";

				/*
				//cout << _unitigSequences[_unitigOrder[from]]._overlapSizePlus << endl;
				//cout << _unitigSequences[_unitigOrder[from]]._overlapSizeMinus << endl;
				
				//cout << _unitigSequences[_unitigOrder[to]]._overlapSizePlus << endl;
				//cout << _unitigSequences[_unitigOrder[to]]._overlapSizeMinus << endl;


				cout << _unitigSequences[_unitigOrder[from]]._seqBitset->to_string() << endl;
				cout << _unitigSequences[_unitigOrder[to]]._seqBitset->to_string() << endl;
				
				if((*fields)[2] == "+"){
					cout << _unitigSequences[_unitigOrder[from]]._overlapSizePlus << endl;
				}
				else{
					cout << _unitigSequences[_unitigOrder[from]]._overlapSizeMinus<< endl;
				}


				if((*fields)[4] == "+"){
					cout << _unitigSequences[_unitigOrder[to]]._overlapSizeMinus<< endl;
				}
				else{
					cout << _unitigSequences[_unitigOrder[to]]._overlapSizePlus << endl;
				}
				*/


				u_int32_t overlapSize = 0;
				
				if((*fields)[2] == "+"){
					overlapSize = _unitigSequences[_unitigOrder[from]]._overlapSizePlus;
				}
				else{
					overlapSize = _unitigSequences[_unitigOrder[from]]._overlapSizeMinus;
				}


				if((*fields)[4] == "+"){
					overlapSize = min(overlapSize, _unitigSequences[_unitigOrder[to]]._overlapSizeMinus);
				}
				else{
					overlapSize = min(overlapSize, _unitigSequences[_unitigOrder[to]]._overlapSizePlus);
				}

				//cout << overlapSize << endl;

				/*
				string& from = (*fields)[1];
				bool fromOrient = (*fields)[2] == "+";
				string& to = (*fields)[3];
				bool toOrient = (*fields)[4] == "+";
				u_int16_t overlap = std::stoull((*fields)[5]);

				u_int32_t from_id = std::stoull(from);
				u_int32_t to_id = std::stoull(to);
				*/

				string line = (*fields)[0] + "\t" + (*fields)[1] + "\t" + (*fields)[2] + "\t" +  (*fields)[3] + "\t" + (*fields)[4] + "\t" + to_string(overlapSize) + "M";
				//cout << line << endl;
				
				outputFile << line << endl;
				outputFileNoSeq << line << endl;

			}

            
        }

        delete fields;
        delete fields_optional;

		outputFile.close();
		outputFileNoSeq.close();


		
		fs::rename(outputFilename, _finalFilename);
		fs::rename(outputFilenameNoSeq, _finalFilenameNoSeq);

	}
};	

#endif 


