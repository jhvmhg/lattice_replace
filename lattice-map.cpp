#include <iostream>
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "lat/kaldi-lattice.h"

template<typename T1, typename T2>
void replace_lat( kaldi::Input &ki, string &line,
                  T1 &lattice_reader,
                  T2 &lattice_writer);


int main(int argc, char *argv[]) {
    using namespace kaldi;
    using namespace std;
    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;
    using fst::SymbolTable;
    using fst::VectorFst;
    using fst::StdArc;

    const char *usage =
            "Copy fake lattices \n"
            "Usage: lattice-map [options] utt-scp-rspecifier  in-lattice-rspecifier lattice-wspecifier\n"
            "e.g.: ./lattice_replace train_fbank_combine_fake/utt.map 'ark: gunzip -c chain/exp/chain/chain_data_all_ori_align_lat/lat.*.gz |' "
            "  'ark,t:chain/exp/chain/chain_data_all_ori_align_lat/text_fake.lats'";

    ParseOptions po(usage);
    bool write_compact = true;
    po.Register("write-compact", &write_compact, "If true, write in normal (compact) form.");

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
        RandomAccessCompactLatticeReader lattice_reader(lats_rspecifier);
        CompactLatticeWriter lattice_writer(lats_wspecifier);


        replace_lat( ki, line, lattice_reader, lattice_writer);

    } else{
        RandomAccessLatticeReader lattice_reader(lats_rspecifier);
        LatticeWriter lattice_writer(lats_wspecifier);


        replace_lat( ki, line, lattice_reader, lattice_writer);
    }



}

template<typename T1, typename T2>
void replace_lat( kaldi::Input &ki, string &line,
                 T1 &lattice_reader,
                 T2 &lattice_writer) {
    while (std::getline(ki.Stream(), line)) {
        std::__1::vector<std::string> split_line;
        kaldi::SplitStringToVector(line, " \t\r", true, &split_line);
        if (split_line.empty()) {
            KALDI_ERR << "Unable to parse line \"" << line << "\" encountered in input in utt_scp_rspecifier";
        }
        lattice_writer.Write(split_line[0], lattice_reader.Value(split_line[1]));
    }
}

