//
// Created by 梅朝阳 on 2020/9/28.
//

// nnet3bin/nnet3-latgen-grammar.cc

// Copyright      2018   Johns Hopkins University (author: Daniel Povey)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.


#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "fstext/fstext-lib.h"
#include "decoder/decoder-wrappers.h"
#include "nnet3/nnet-am-decodable-simple.h"
#include "nnet3/nnet-utils.h"
#include "decoder/grammar-fst.h"
#include "base/timer.h"

using namespace fst;


// Reads an FST from disk using Kaldi I/O mechanisms, and if it is not of type
// ConstFst, copies it to that stype.
ConstFst<StdArc>* ReadAsConstFst(std::string rxfilename) {
    // the following call will throw if there is an error.
    Fst<StdArc> *fst = ReadFstKaldiGeneric(rxfilename);
    ConstFst<StdArc> *const_fst = dynamic_cast<ConstFst<StdArc>* >(fst);
    if (!const_fst) {
        const_fst = new ConstFst<StdArc>(*fst);
        delete fst;
    }
    return const_fst;
}


int main(int argc, char *argv[]) {
    // note: making this program work with GPUs is as simple as initializing the
    // device, but it probably won't make a huge difference in speed for typical
    // setups.
    try {
        using namespace kaldi;
        using namespace kaldi::nnet3;
        typedef kaldi::int32 int32;
        using fst::SymbolTable;
        using fst::Fst;
        using fst::StdArc;

        const char *usage =
                "Generate lattices using nnet3 neural net model, and GrammarFst-based graph\n"
                "see kaldi-asr.org/doc/grammar.html for more context.\n"
                "\n"
                "Usage: nnet3-latgen-grammar [options] <nnet-in> <grammar-fst-in> <features-rspecifier>"
                " <lattice-wspecifier> [ <words-wspecifier> [<alignments-wspecifier>] ]\n";

        int32 nonterm_phones_offset = -1;

        ParseOptions po(usage);
        Timer timer;
        bool allow_partial = false;
        LatticeFasterDecoderConfig config;
        NnetSimpleComputationOptions decodable_opts;
        int32 online_ivector_period = 0;

        std::string word_syms_filename;

        config.Register(&po);
        decodable_opts.Register(&po);
        po.Register("word-symbol-table", &word_syms_filename,
                    "Symbol table for words [for debug output]");
        po.Register("allow-partial", &allow_partial,
                    "If true, produce output even if end state was not reached.");
        po.Register("nonterm-phones-offset", &nonterm_phones_offset,
                    "Integer id of #nonterm_bos in phones.txt");


        po.Read(argc, argv);

        if (po.NumArgs() < 6 || po.NumArgs() > 8) {
            po.PrintUsage();
            exit(1);
        }

        std::string model_rxfilename = po.GetArg(1),
                top_fst_str = po.GetArg(2),
                nonterm_str = po.GetArg(3),
                little_fst_rxfilename = po.GetArg(4),
                feature_rspecifier = po.GetArg(5),
                lattice_wspecifier = po.GetOptArg(6),
                words_wspecifier = po.GetOptArg(7),
                alignment_wspecifier= po.GetOptArg(8);


        // 没用的
        Int32VectorWriter alignment_writer(alignment_wspecifier);
        CompactLatticeWriter compact_lattice_writer;
        LatticeWriter lattice_writer;
        compact_lattice_writer.Open(lattice_wspecifier);

        TransitionModel trans_model;
        AmNnetSimple am_nnet;
        {
            bool binary;
            Input ki(model_rxfilename, &binary);
            trans_model.Read(ki.Stream(), binary);
            am_nnet.Read(ki.Stream(), binary);
            SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
            SetDropoutTestMode(true, &(am_nnet.GetNnet()));
            CollapseModel(CollapseModelConfig(), &(am_nnet.GetNnet()));
        }





        Int32VectorWriter words_writer(words_wspecifier);

        fst::SymbolTable *word_syms = NULL;
        if (word_syms_filename != "")
            if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
                KALDI_ERR << "Could not read symbol table from file "
                          << word_syms_filename;

        double tot_like = 0.0;
        kaldi::int64 frame_count = 0;
        int num_success = 0, num_fail = 0;
        // this compiler object allows caching of computations across
        // different utterances.
        CachingOptimizingCompiler compiler(am_nnet.GetNnet(),
                                           decodable_opts.optimize_config);

        SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);


        //构建grammar_fst

        std::shared_ptr<const ConstFst<StdArc> > top_fst(
                ReadAsConstFst(top_fst_str));
        std::vector<std::pair<int32, std::shared_ptr<const ConstFst<StdArc> > > > pairs;
        int32 nonterminal;
        ConvertStringToInteger(nonterm_str, &nonterminal);
        std::shared_ptr<const ConstFst<StdArc> > this_fst(ReadAsConstFst(little_fst_rxfilename));
        pairs.push_back(std::pair<int32, std::shared_ptr<const ConstFst<StdArc> > >(
                nonterminal, this_fst));

        fst::GrammarFst* grammar_fst = new fst::GrammarFst(nonterm_phones_offset,
                                                           top_fst,
                                                           pairs);
        timer.Reset();

        {
            LatticeFasterDecoderTpl<fst::GrammarFst> decoder(*grammar_fst, config);

            for (; !feature_reader.Done(); feature_reader.Next()) {
                std::string utt = feature_reader.Key();
                const Matrix<BaseFloat> &features (feature_reader.Value());
                if (features.NumRows() == 0) {
                    KALDI_WARN << "Zero-length utterance: " << utt;
                    num_fail++;
                    continue;
                }
                const Matrix<BaseFloat> *online_ivectors = NULL;
                const Vector<BaseFloat> *ivector = NULL;


                DecodableAmNnetSimple nnet_decodable(
                        decodable_opts, trans_model, am_nnet,
                        features, ivector, online_ivectors,
                        online_ivector_period, &compiler);

                double like;
                if (DecodeUtteranceLatticeFaster(
                        decoder, nnet_decodable, trans_model, word_syms, utt,
                        decodable_opts.acoustic_scale, true, allow_partial,
                        &alignment_writer, &words_writer, &compact_lattice_writer,
                        &lattice_writer,
                        &like)) {
                    tot_like += like;
                    frame_count += nnet_decodable.NumFramesReady();
                    num_success++;
                } else num_fail++;
            }
        }

        kaldi::int64 input_frame_count =
                frame_count * decodable_opts.frame_subsampling_factor;

        double elapsed = timer.Elapsed();
        KALDI_LOG << "Time taken "<< elapsed
                  << "s: real-time factor assuming 100 frames/sec is "
                  << (elapsed * 100.0 / input_frame_count);
        KALDI_LOG << "Done " << num_success << " utterances, failed for "
                  << num_fail;
        KALDI_LOG << "Overall log-likelihood per frame is "
                  << (tot_like / frame_count) << " over "
                  << frame_count << " frames.";

        delete word_syms;
        if (num_success != 0) return 0;
        else return 1;
    } catch(const std::exception &e) {
        std::cerr << e.what();
        return -1;
    }
}

