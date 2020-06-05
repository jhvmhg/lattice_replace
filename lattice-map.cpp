#include <iostream>
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "lat/kaldi-lattice.h"

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
            "Usage: lattice-map [options] utt-scp-rspecifier  in-lattice-rspecifier lattice-wspecifier\n";

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


    if (write_compact){
        RandomAccessCompactLatticeReader lattice_reader(lats_rspecifier);
        CompactLatticeWriter lattice_writer(lats_wspecifier);

        bool binary;
        Input ki(utt_scp_rspecifier, &binary);
        KALDI_ASSERT(!binary);
        std::string line;
        while (std::getline(ki.Stream(), line)) {
            std::vector<std::string> split_line;
            SplitStringToVector(line, " \t\r", true, &split_line);
            if (split_line.empty()) {
                KALDI_ERR << "Unable to parse line \"" << line << "\" encountered in input in " << utt_scp_rspecifier;
            }
            lattice_writer.Write(split_line[0], lattice_reader.Value(split_line[1]));
        }

    } else{
        RandomAccessLatticeReader lattice_reader(lats_rspecifier);
        LatticeWriter lattice_writer(lats_wspecifier);

        bool binary;
        Input ki(utt_scp_rspecifier, &binary);
        KALDI_ASSERT(!binary);
        std::string line;
        while (std::getline(ki.Stream(), line)) {
            std::vector<std::string> split_line;
            SplitStringToVector(line, " \t\r", true, &split_line);
            if (split_line.empty()) {
                KALDI_ERR << "Unable to parse line \"" << line << "\" encountered in input in " << utt_scp_rspecifier;
            }
            lattice_writer.Write(split_line[0], lattice_reader.Value(split_line[1]));
        }
    }



}

