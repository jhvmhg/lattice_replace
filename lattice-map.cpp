#include <iostream>
#include <map>
#include <string>
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "lat/kaldi-lattice.h"

using namespace kaldi;
using namespace std;


template<typename T1, typename T2>
void replace_lat_with_lattice_in( kaldi::Input &ki, bool ignore_lack,
                  T1 &lattice_reader,
                  T2 &lattice_writer);



template<typename T1, typename T2>
void replace_lat_with_utt_scp( kaldi::Input &ki, bool ignore_lack,
                  T1 &lattice_reader,
                  T2 &lattice_writer);


int main(int argc, char *argv[]) {

    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;
    using fst::SymbolTable;
    using fst::VectorFst;
    using fst::StdArc;

    const char *usage =
            "Copy fake lattices ,utt-scp-rspecifier format is 'new_utt  original_utt'\n"
            "Usage: lattice-map [options] utt-scp-rspecifier in-lattice-rspecifier lattice-wspecifier\n"
            "e.g.: ./lattice_replace train_fbank_combine_fake/utt.map 'ark: gunzip -c chain/exp/chain/chain_data_all_ori_align_lat/lat.*.gz |' "
            "  'ark,t:chain/exp/chain/chain_data_all_ori_align_lat/text_fake.lats'";

    ParseOptions po(usage);
    bool write_compact = true;
    bool ignore_lack = true;
    bool write_from_lattice_in = false;
    po.Register("write-compact", &write_compact, "If true, write in normal (compact) form.");
    po.Register("utt-from-in-lattice", &write_from_lattice_in, "If true, write in normal (compact) form.");
    po.Register("ignore-lack", &ignore_lack, "If true, When in-lattice no such utt's lattice , "
                                                        " just warn and skip it.");

    po.Read(argc, argv);

    if (po.NumArgs() != 3) {
        po.PrintUsage();
        exit(1);
    }
    std::string utt_scp_rspecifier = po.GetArg(1),
            lats_rspecifier = po.GetArg(2),
            lats_wspecifier = po.GetArg(3);

    bool binary;
    Input ki(utt_scp_rspecifier, &binary);
    KALDI_ASSERT(!binary);
    std::string line;
    if (write_compact){

        if ( write_from_lattice_in){
            SequentialCompactLatticeReader lattice_reader(lats_rspecifier);
            CompactLatticeWriter lattice_writer(lats_wspecifier);
            replace_lat_with_lattice_in( ki, ignore_lack,lattice_reader, lattice_writer);
        } else{
            RandomAccessCompactLatticeReader lattice_reader(lats_rspecifier);
            CompactLatticeWriter lattice_writer(lats_wspecifier);
            replace_lat_with_utt_scp(ki, ignore_lack,lattice_reader, lattice_writer);
        }


    } else{



        if ( write_from_lattice_in){
            SequentialLatticeReader lattice_reader(lats_rspecifier);
            LatticeWriter lattice_writer(lats_wspecifier);
            replace_lat_with_lattice_in( ki, ignore_lack,lattice_reader, lattice_writer);
        } else{
            RandomAccessLatticeReader lattice_reader(lats_rspecifier);
            LatticeWriter lattice_writer(lats_wspecifier);
            replace_lat_with_utt_scp(ki, ignore_lack,lattice_reader, lattice_writer);
        }

    }



}


template<typename T1, typename T2>
void replace_lat_with_lattice_in(Input &ki, bool ignore_lack, T1 &lattice_reader, T2 &lattice_writer) {
    string line;
    map<string, string> mapUtt;
    while (std::getline(ki.Stream(), line)) {
        vector<string> split_line;
        SplitStringToVector(line, " \t\r", true, &split_line);
        mapUtt.insert(pair<string, string>(split_line[1], split_line[0]));
    }
    string new_utt;
    for (; !lattice_reader.Done(); lattice_reader.Next()){
        map<string, string>::iterator iter = mapUtt.find(lattice_reader.Key());
        if (iter != mapUtt.end()){
            new_utt = iter->second;
            lattice_writer.Write(new_utt, lattice_reader.Value());
        } else{
            KALDI_WARN << "no such utt in utt_scp_rspecifier";
        }

    }


}

template<typename T1, typename T2>
void replace_lat_with_utt_scp(Input &ki, bool ignore_lack, T1 &lattice_reader, T2 &lattice_writer) {
    string line;
    while (std::getline(ki.Stream(), line)) {
        vector<string> split_line;
        SplitStringToVector(line, " \t\r", true, &split_line);
        if (split_line.empty())
            KALDI_ERR << "Unable to parse line \"" << line << "\" encountered in input in utt_scp_rspecifier";
        if (lattice_reader.HasKey(split_line[1])){
            lattice_writer.Write(split_line[0], lattice_reader.Value(split_line[1]));
        } else {
            if (ignore_lack) KALDI_WARN << "No such key: " << split_line[1];
            else KALDI_ERR << "No such key: " << split_line[1];
        }
    }
}
