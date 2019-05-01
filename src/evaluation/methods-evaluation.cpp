#include <stdlib.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cassert>
#include <random>
#include <iomanip>

#include "fsm/Dfsm.h"
#include "fsm/Fsm.h"
#include "fsm/InputTrace.h"
#include "fsm/IOTrace.h"
#include "interface/FsmPresentationLayer.h"
#include "trees/IOListContainer.h"
#include "trees/InputOutputTree.h"
#include "trees/SplittingTree.h"
#include "trees/IOTreeContainer.h"
#include "utils/Logger.hpp"

using namespace std;

unsigned int getRandomSeed() {

    return static_cast<unsigned int>
    (std::chrono::high_resolution_clock::now().time_since_epoch().count());

}

std::mt19937 rand_engine (getRandomSeed());  // replace knuth_b with one of the engines listed below
// std::uniform_real_distribution<> uniform_zero_to_one(0.0, 1.0);

bool random_bool_with_prob( double prob )  // probability between 0.0 and 1.0
{
    std::bernoulli_distribution d(prob);
    return d(rand_engine);
}

shared_ptr<FsmPresentationLayer> createPresentationLayer(const size_t maxInput, const size_t refMin, const size_t maxOutput)
{
    vector<string> in2string = vector<string>();
    for (size_t i = 0; i <= maxInput; i++)
    {
        in2string.push_back(to_string(i));
    }
    vector<string> out2string = vector<string>();
    for (size_t i = 0; i <= maxOutput; i++)
    {
        out2string.push_back(to_string(i));
    }
    vector<string>state2string = vector<string>();
    for (size_t i = 0; i < refMin; i++)
    {
        state2string.push_back(to_string(i));
    }
    shared_ptr<FsmPresentationLayer> pl{
            new FsmPresentationLayer(
                    in2string,
                    out2string,
                    state2string)};

    return pl;
}
bool randomBool() {
    return rand() > (RAND_MAX / 2);
}

void testLeeAds()
{
    cout << "Starting ADS Test for lee94_no_pds.fsm..." << endl;
    shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(1,6,1);
    shared_ptr<Dfsm> dfsm = make_shared<Dfsm>("../../../resources/lee94_no_pds.fsm",pl,"lee94_no_pds");

    vector<int> ds = dfsm->createDistinguishingSequence();

    //lee94_no_pds does not possess a pds
    assert(ds.empty());

    shared_ptr<InputOutputTree> ads = dfsm->createAdaptiveDistinguishingSequence();

    //lee94_no_pds does possess an ads
    assert(ads);

    auto nds = dfsm->getNodes();
    auto& nodes = nds;

    auto adsList = make_shared<vector<shared_ptr<InputOutputTree>>>();
    adsList->push_back(ads);
    IOTreeContainer adaptiveTestCases(adsList,dfsm->getPresentationLayer());

    //the ads should distinguish all states from each other
    assert(dfsm->distinguishesAllStates(nodes,nodes,adaptiveTestCases));
    cout << "Finished with SUCCESS!" << endl << endl;
}

void testRandomPdsAndAds(const int numStates,const int numInput,const int numOutput)
{
    cout << "Starting PDS and ADS creation Test for with random DFSM..." << endl;

    int numberOfTests = 100;

    for(int i=0;i<numberOfTests;++i) {
        shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(numInput, numStates, numOutput);
        //create random minimised dfsm with certain number of states, inputs and outputs
        shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", numStates, numInput, numOutput, pl)->minimise());

        vector<int> ds = dfsm->createDistinguishingSequence();
        while(ds.empty()) {
            dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", numStates, numInput, numOutput, pl)->minimise());
            ds = dfsm->createDistinguishingSequence();
        }

        shared_ptr<InputOutputTree> ads = dfsm->createAdaptiveDistinguishingSequence();

        //ads should exist for dfsm with pds
        assert(ads);

        auto nds = dfsm->getNodes();
        auto& nodes = nds;

        InputTrace distinguishingSequence(ds,pl);

        //distinguishing sequence should distinguish all states from each other
        assert(dfsm->distinguishesAllStates(nodes,distinguishingSequence));

        auto adsList = make_shared<vector<shared_ptr<InputOutputTree>>>();
        adsList->push_back(ads);
        IOTreeContainer adaptiveTestCases(adsList,dfsm->getPresentationLayer());

        //adaptive distinguishing sequence should distinguish all states from each other
        assert(dfsm->distinguishesAllStates(nodes,nodes,adaptiveTestCases));
    }
    cout << "Finished with SUCCESS!" << endl << endl;
}

void evaluateDMethodsFaultCoverage() {

    cout << " Start D-Methods Fault Coverage Evaluation..." << endl;

    int numDfsm = 10,
        numMutants = 100;

    vector<vector<int>> dfsm_sizes = {{10,2,2},
                                      {10,3,3},
                                      {20,4,4},
                                      {20,6,6},
                                      {30,6,6},
                                      {30,8,8},
                                      {50,10,10},
                                      {50,13,13},
                                      {75,15,15},
                                      {75,18,18},
                                      {100,20,20},
                                      {100,24,24}
                                      };
    ofstream out("dmethods_fc.csv");
    out << "states  ,"
        << "inputs  ,"
        << "outputs ,"
        << "unequal mutants ,"
        << "not pass. D-Method,"
        << "not pass. D-Method(ADS),"
        << "not pass. Hierons D-Method,"
        << "not pass. Hierons D-Method(ADS),"
        << "D-Method Fault Coverage,"
        << "D-Method(ADS) Fault Coverage,"
        << "Hierons D-Method Fault Coverage,"
        << "Hierons D-Method(ADS) Fault Coverage" << endl;

    for(auto& dfsm_size:dfsm_sizes) {
        cout << endl << "Evaluate Fault Coverage for DFSM with: " << endl
             << "\t -states -> " << to_string(dfsm_size[0]) << endl
             << "\t -inputs -> " << to_string(dfsm_size[1]) << endl
             << "\t -outputs -> " << to_string(dfsm_size[2]) << endl;

        float sidPds_fc = 0,
              sidAds_fc = 0,
              hierPds_fc = 0,
              hierAds_fc = 0;

        int num_nonequal_mut = 0,
                num_not_passing_sidPds = 0,
                num_not_passing_sidAds = 0,
                num_not_passing_hierPds = 0,
                num_not_passing_hierAds = 0;

        for(int i=0;i<numDfsm;++i) {

            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());

            vector<int> ds = dfsm->createDistinguishingSequence();
            while(ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                        dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }

            cout << "Created next random DFSM(" << i << ")..." << endl;

            auto a = dfsm->dMethodOnMinimisedDfsm(0,false);
            vector<IOTrace> sidPdsTs;
            for(vector<int> v: *a.getIOLists()) {
                InputTrace i(v,pl);
                IOTrace io = dfsm->applyDet(i);
                sidPdsTs.push_back(io);
            }

            auto b = dfsm->dMethodOnMinimisedDfsm(0,true);
            vector<IOTrace> sidAdsTs;
            for(vector<int> v: *b.getIOLists()) {
                InputTrace i(v,pl);
                IOTrace io = dfsm->applyDet(i);
                sidAdsTs.push_back(io);
            }

            auto c = dfsm->hieronsDMethodOnMinimisedDfsm(false);
            vector<IOTrace> hierPdsTs;
            for(vector<int> v: *c.getIOLists()) {
                InputTrace i(v,pl);
                IOTrace io = dfsm->applyDet(i);
                hierPdsTs.push_back(io);
            }

            auto d = dfsm->hieronsDMethodOnMinimisedDfsm(false);
            vector<IOTrace> hierAdsTs;
            for(vector<int> v: *c.getIOLists()) {
                InputTrace i(v,pl);
                IOTrace io = dfsm->applyDet(i);
                hierAdsTs.push_back(io);
            }
            cout << "Created testsuites for all D-Method variants..." << endl;

            for(int j=0;j<numMutants;++j) {
                unsigned int numOutputFaults = rand() % (dfsm->getMaxNodes()/3),
                        numTransitionFaults = rand() % (dfsm->getMaxNodes()/3);
                if(random_bool_with_prob(0.4)) {
                    numOutputFaults = 0;
                    numTransitionFaults = 0;
                }
                shared_ptr<Dfsm> mutant = dfsm->createMutant("Mutant",numOutputFaults,numTransitionFaults);
                //cout << "Created next random Mutant(" << j << ") with: "
                //     << "\t -num. output faults -> " << numOutputFaults
                //     << "\t -num. transition faults -> " << numTransitionFaults << endl;

                bool result = dfsm->equivalenceCheck(*mutant);
                if (!result) num_nonequal_mut++;

                result = true;
                for (IOTrace io: sidPdsTs) {
                    result &= mutant->pass(io);
                }
                if (!result) num_not_passing_sidPds++;

                result = true;
                for (IOTrace io: sidAdsTs) {
                    result &= mutant->pass(io);
                }
                if (!result) num_not_passing_sidAds++;

                result = true;
                for (IOTrace io: hierPdsTs) {
                    result &= mutant->pass(io);
                }
                if (!result) num_not_passing_hierPds++;

                result = true;
                for (IOTrace io: hierAdsTs) {
                    result &= mutant->pass(io);
                }
                if (!result) num_not_passing_hierAds++;

                //cout << "All testsuites executed on the mutant!" << endl;
            }
        }

        sidPds_fc = (num_nonequal_mut>0?(float)num_not_passing_sidPds/(float)num_nonequal_mut*100:
                   ((num_not_passing_sidPds == 0)?100:0));
        sidAds_fc = (num_nonequal_mut>0?(float)num_not_passing_sidAds/(float)num_nonequal_mut*100:
                     ((num_not_passing_sidAds == 0)?100:0));
        hierPds_fc = (num_nonequal_mut>0?(float)num_not_passing_hierPds/(float)num_nonequal_mut*100:
                     ((num_not_passing_hierPds == 0)?100:0));
        hierAds_fc = (num_nonequal_mut>0?(float)num_not_passing_hierAds/(float)num_nonequal_mut*100:
                      ((num_not_passing_hierAds == 0)?100:0));

        out << dfsm_size[0] << ","
            << dfsm_size[1] << ","
            << dfsm_size[2] << ","
            << num_nonequal_mut << ","
            << num_not_passing_sidPds << ","
            << num_not_passing_sidAds << ","
            << num_not_passing_hierPds << ","
            << num_not_passing_hierAds << ","
            << sidPds_fc << ","
            << sidAds_fc << ","
            << hierPds_fc << ","
            << hierAds_fc << endl;
    }
    out.close();
    cout << "Finished!" << endl << endl;
}

void evaluateDMethodsApplicability() {

    cout << " Start D-Methods Applicability Evaluation..." << endl;

    int numDfsm = 100;

    vector<vector<int>> dfsm_sizes = {{20,1,1}, //0.1
                                      {20,2,2}, //0.15
                                      {20,3,3}, //0.2
                                      {20,4,4}, //0.25
                                      {20,10,10}, //0.55
                                      {30,2,2}, //0.1
                                      {30,3,3}, //0.13333
                                      {30,5,5}, //0.2
                                      {30,6,6}, //0.2333
                                      {30,16,16}, //0.566
                                      {50,4,4}, //0.1
                                      {50,7,7}, //0.16
                                      {50,9,9}, //0.2
                                      {50,11,11}, //0.24
                                      {50,26,26}, //0.54
                                      {70,6,6}, //0.1
                                      {70,10,10}, //0.157
                                      {70,13,13}, //0.2
                                      {70,16,16}, //0.242
                                      {70,35,35}, //0.514
                                      {100,9,9}, //0.1
                                      {100,14,14}, //0.15
                                      {100,19,19}, //0.2
                                      {100,23,23}, //0.24
                                      {100,50,50}, //0.51
    };
    ofstream out("dmethods_ac.csv");
    out << "No. Dfsm,"
        << "states,"
        << "inputs,"
        << "outputs,"
        << "ratio(output/state),"
        << "D-Method,"
        << "D-Method(ADS), "
        << "Hierons D-Method,"
        << "Hierons D-Method(ADS)"
        << endl;

    for(auto& dfsm_size:dfsm_sizes) {
        cout << endl << "Evaluate Applicability for DFSM with: " << endl
             << "\t -states -> " << to_string(dfsm_size[0]) << endl
             << "\t -inputs -> " << to_string(dfsm_size[1]) << endl
             << "\t -outputs -> " << to_string(dfsm_size[2]) << endl;

        float sidPds_ac = 0,
                sidAds_ac = 0,
                hierPds_ac = 0,
                hierAds_ac = 0;

        for(int i=0;i<numDfsm;++i) {

            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }

            cout << "Created next random DFSM(" << i << ")..." << endl;

            auto a = dfsm->dMethodOnMinimisedDfsm(0,false);
            if(a.size() > 0) sidPds_ac++;

            auto b = dfsm->dMethodOnMinimisedDfsm(0,true);
            if(b.size() > 0) sidAds_ac++;

            /*auto c = dfsm->hieronsDMethodOnMinimisedDfsm(false);
            if(c.size() > 0) hierPds_ac++;

            auto d = dfsm->hieronsDMethodOnMinimisedDfsm(true);
            if(d.size() > 0) hierAds_ac++;
            */
            cout << "Created testsuites for all D-Method variants..." << endl;
        }

        sidPds_ac = (float)sidPds_ac/(float)numDfsm*100;
        sidAds_ac = (float)sidAds_ac/(float)numDfsm*100;
        hierPds_ac = (float)hierPds_ac/(float)numDfsm*100;
        hierAds_ac = (float)hierAds_ac/(float)numDfsm*100;

        out << numDfsm << ","
            << dfsm_size[0] << ","
            << (dfsm_size[1]+1) << ","
            << (dfsm_size[2]+1) << ","
            << ((float) (dfsm_size[2]+1)/ (float) dfsm_size[0]) << ","
            << sidPds_ac << ","
            << sidAds_ac << ","
            << hierPds_ac << ","
            << hierAds_ac
            << endl;
    }
    out.close();
    cout << "Finished!" << endl << endl;
}

void evaluateTestSuiteSizes() {
    cout << " Start Test Suite Size Evaluation..." << endl;

    int numDfsm = 50;

    vector<vector<int>> dfsm_sizes = {{10,1,1}, //20
                                      {10,2,2}, //30
                                      {10,3,3}, //40
                                      {20,2,2}, //60
                                      {20,3,3}, //80
                                      {20,4,4}, //100
                                      {20,5,5}, //120
                                      {30,4,4}, //150
                                      {30,5,5}, //180
                                      {30,6,6}, //210
                                      {50,4,4}, //250
                                      {50,6,6}, //350
                                      {50,7,7}, //400
                                      {50,8,8}, //450
                                      {70,7,7}, //560
                                      {70,8,8}, //630
                                      {70,9,9}, //700
                                      {70,11,11}, //840
                                      {80,13,13}, //1120
                                      {100,9,9}, //1000
                                      {100,11,11}, //1200
                                      {100,12,12}, //1300
    };

    std::cout << std::fixed << std::showpoint;
    std::cout << std::setprecision(2);

    ofstream out("testsuite_sizes.csv");
    out << "states  ,"
        << "inputs  ,"
        << "outputs ,"
        << "transitions ,"
        << "W-Method,"
        << "w Duration,"
        << "Wp-Method,"
        << "wp Duration,"
        << "D-Method,"
        << "D Duration,"
        << "D-Method(ADS),"
        << "D (ADS) Duration,"
        << "Hierons D-Method,"
        << "Hier. D Duration,"
        << "Hierons D-Method(ADS),"
        << "Hier. D (ADS) Duration,"
        << "T-Method,"
        << "T Duration" << endl;

    for(auto& dfsm_size:dfsm_sizes) {
        cout << endl << "Evaluate Test suite size for DFSM with: " << endl
             << "\t -states -> " << to_string(dfsm_size[0]) << endl
             << "\t -inputs -> " << to_string(dfsm_size[1]) << endl
             << "\t -outputs -> " << to_string(dfsm_size[2]) << endl;

        double avgWMethSize = 0,
            avgWMethDuration = 0,
            avgWpMethSize = 0,
            avgWpMethDuration = 0,
            avgDMethSize = 0,
            avgDMethDuration = 0,
            avgADSMethSize = 0,
            avgADSMethDuration = 0,
            avgHDMethSize = 0,
            avgHDMethDuration = 0,
            avgHADSMethSize = 0,
            avgHADSMethDuration = 0,
            avgTMethSize = 0,
            avgTMethDuration = 0;

        cout << "Evaluate Testsuite size for W-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto start = chrono::high_resolution_clock::now();

            auto ts = dfsm->wMethodOnMinimisedDfsm(0);

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            avgWMethDuration += elapsed.count();
            avgWMethSize += ts.getFlatSize();
        }
        avgWMethDuration = (double) avgWMethDuration/(double) numDfsm;
        avgWMethSize = (double) avgWMethSize/(double) numDfsm;

        cout << "Evaluate Testsuite size for Wp-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto start = chrono::high_resolution_clock::now();

            auto ts = dfsm->wpMethodOnMinimisedDfsm(0);

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            avgWpMethDuration += elapsed.count();
            avgWpMethSize += ts.getFlatSize();
        }
        avgWpMethDuration = (double) avgWpMethDuration/(double) numDfsm;
        avgWpMethSize = (double) avgWpMethSize/(double) numDfsm;

        cout << "Evaluate Testsuite size for T-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto start = chrono::high_resolution_clock::now();

            auto ts = dfsm->tMethod();

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            avgTMethDuration += elapsed.count();
            avgTMethSize += ts.getFlatSize();
        }
        avgTMethDuration = (double) avgTMethDuration/(double) numDfsm;
        avgTMethSize = (double) avgTMethSize/(double) numDfsm;

        cout << "Evaluate Testsuite size for D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto start = chrono::high_resolution_clock::now();

            auto ts = dfsm->dMethodOnMinimisedDfsm(0,false);

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            avgDMethDuration += elapsed.count();
            avgDMethSize += ts.getFlatSize();
        }
        avgDMethDuration = (double) avgDMethDuration/(double) numDfsm;
        avgDMethSize = (double) avgDMethSize/(double) numDfsm;

        cout << "Evaluate Testsuite size for D-Method(ADS):" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto start = chrono::high_resolution_clock::now();

            auto ts = dfsm->dMethodOnMinimisedDfsm(0,true);

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            avgADSMethDuration += elapsed.count();
            avgADSMethSize += ts.getFlatSize();
        }
        avgADSMethDuration = (double) avgADSMethDuration/(double) numDfsm;
        avgADSMethSize = (double) avgADSMethSize/(double) numDfsm;

        cout << "Evaluate Testsuite size for Hierons D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto start = chrono::high_resolution_clock::now();

            auto ts = dfsm->hieronsDMethodOnMinimisedDfsm(false);

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            avgHDMethDuration += elapsed.count();
            avgHDMethSize += ts.getFlatSize();
        }
        avgHDMethDuration = (double) avgHDMethDuration/(double) numDfsm;
        avgHDMethSize = (double) avgHDMethSize/(double) numDfsm;

        cout << "Evaluate Testsuite size for Hierons D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto start = chrono::high_resolution_clock::now();

            auto ts = dfsm->hieronsDMethodOnMinimisedDfsm(true);

            auto finish = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = finish - start;

            avgHADSMethDuration += elapsed.count();
            avgHADSMethSize += ts.getFlatSize();
        }
        avgHADSMethDuration = (double) avgHADSMethDuration/(double) numDfsm;
        avgHADSMethSize = (double) avgHADSMethSize/(double) numDfsm;

        out << dfsm_size[0] << ","
            << (dfsm_size[1]+1) << ","
            << (dfsm_size[2]+1) << ","
            << (dfsm_size[0]) * (dfsm_size[1]+1) << ","
            << avgWMethSize << ","
            << avgWMethDuration << ","
            << avgWpMethSize << ","
            << avgWpMethDuration << ","
            << avgDMethSize << ","
            << avgDMethDuration << ","
            << avgADSMethSize << ","
            << avgADSMethDuration << ","
            << avgHDMethSize << ","
            << avgHDMethDuration << ","
            << avgHADSMethSize << ","
            << avgHADSMethDuration << ","
            << avgTMethSize << ","
            << avgTMethDuration << endl;
    }
    out.close();
    cout << "Finished!" << endl << endl;
}

int calcNumAddStatesFCTreshold(IOListContainer& ts,double fcTreshold,int numMutants,
        shared_ptr<FsmPresentationLayer>& pl,shared_ptr<Dfsm>& dfsm) {

    vector<IOTrace> traces;
    for(vector<int> v: *ts.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        traces.push_back(io);
    }

    double fc = 100;
    int numAddStates = 0;

    while(fc >= fcTreshold) {
        ++numAddStates;
        //cout << "numAddStates -> " << numAddStates << endl;

        int num_non_equal = 0;
        int num_not_pass = 0;
        fc = 0;
        for(int j=0;j<numMutants;++j) {
            //unsigned int numOutputFaults = rand() % (dfsm->size()/3),
             //       numTransitionFaults = rand() % (dfsm->size()/3);

            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,numAddStates)->minimise());
            //cout << "mutant number " << j << endl;
            while(mutant->size() < (dfsm->size() + numAddStates)) {
                //cout << "try another mut" << endl;
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,numAddStates)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_non_equal++;

            result = true;
            for (IOTrace io: traces) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        //cout << "num not pass -> " << num_not_pass << ", num non equal -> " << num_non_equal << endl;
        fc = (num_non_equal>0?(double)num_not_pass/(double)num_non_equal*100:
                     ((num_not_pass == 0)?100:0));
        //cout << "fault coverage -> " << fc << endl;

    }

    return numAddStates;
}

void evaluateFCOutsideFaultDomain() {
    cout << " Start Fault Coverage Evaluation for Additional States(Outside of the Faul Domain)..." << endl;

    int numDfsm = 20;
    int numMuts = 40;
    double faultCoverageTreshhold = 60;

    vector<vector<int>> dfsm_sizes = {{10,1,1}, //20
                                      {10,2,2}, //30
                                      {10,3,3}, //40
                                      {20,2,2}, //60
                                      {20,3,3}, //80
                                      {20,4,4}, //100
                                      {20,5,5}, //120
                                      {30,4,4}, //150
                                      {30,5,5}, //180
                                      {30,6,6}, //210
                                      {50,4,4}, //250
                                      {50,6,6}, //350
                                      {50,7,7}, //400
                                      {50,8,8}, //450
                                      {70,7,7}, //560
                                      {70,8,8}, //630
                                      {70,9,9}, //700
                                      {70,11,11}, //840
                                      {80,13,13}, //1120
                                      {100,9,9}, //1000
                                      {100,11,11}, //1200
                                      {100,12,12}, //1300
    };

    std::cout << std::fixed << std::showpoint;
    std::cout << std::setprecision(2);

    ofstream out("fc_for_add_states.csv");
    out << "states  ,"
        << "inputs  ,"
        << "outputs ,"
        << "transitions ,"
        << "W-Method,"
        << "Wp-Method,"
        << "D-Method,"
        << "D-Method(ADS),"
        << "Hierons D-Method,"
        << "Hierons D-Method(ADS)" << endl;

    for(auto& dfsm_size:dfsm_sizes) {
        cout << endl << "Evaluate Test suite size for DFSM with: " << endl
             << "\t -states -> " << to_string(dfsm_size[0]) << endl
             << "\t -inputs -> " << to_string(dfsm_size[1]) << endl
             << "\t -outputs -> " << to_string(dfsm_size[2]) << endl;

        double avgWMethodAddStates = 0,
                avgWpMethodAddStates = 0,
                avgDMethodAddStates = 0,
                avgADSMethodAddStates = 0,
                avgHDMethodAddStates = 0,
                avgHADSMethodAddStates = 0;

        cout << "Evaluate Fault Coverage for W-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->wMethodOnMinimisedDfsm(0);

            avgWMethodAddStates += calcNumAddStatesFCTreshold(ts,faultCoverageTreshhold,numMuts,pl,dfsm);
        }
        avgWMethodAddStates = (double) avgWMethodAddStates/(double) numDfsm;

        cout << "Evaluate Fault Coverage for Wp-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->wpMethodOnMinimisedDfsm(0);

            avgWpMethodAddStates += calcNumAddStatesFCTreshold(ts,faultCoverageTreshhold,numMuts,pl,dfsm);
        }
        avgWpMethodAddStates = (double) avgWpMethodAddStates/(double) numDfsm;

        cout << "Evaluate Fault Coverage for D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->dMethodOnMinimisedDfsm(0,false);

            avgDMethodAddStates += calcNumAddStatesFCTreshold(ts,faultCoverageTreshhold,numMuts,pl,dfsm);
        }
        avgDMethodAddStates = (double) avgDMethodAddStates/(double) numDfsm;

        cout << "Evaluate Fault Coverage for D-Method(ADS):" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->dMethodOnMinimisedDfsm(0,true);

            avgADSMethodAddStates += calcNumAddStatesFCTreshold(ts,faultCoverageTreshhold,numMuts,pl,dfsm);
        }
        avgADSMethodAddStates = (double) avgADSMethodAddStates/(double) numDfsm;

        cout << "Evaluate Fault Coverage for Hierons D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->hieronsDMethodOnMinimisedDfsm(false);

            avgHDMethodAddStates += calcNumAddStatesFCTreshold(ts,faultCoverageTreshhold,numMuts,pl,dfsm);
        }
        avgHDMethodAddStates = (double) avgHDMethodAddStates/(double) numDfsm;

        cout << "Evaluate Fault Coverage for Hierons D-Method(ADS):" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->hieronsDMethodOnMinimisedDfsm(true);

            avgHADSMethodAddStates += calcNumAddStatesFCTreshold(ts,faultCoverageTreshhold,numMuts,pl,dfsm);
        }
        avgHADSMethodAddStates = (double) avgHADSMethodAddStates/(double) numDfsm;
        out << dfsm_size[0] << ","
            << (dfsm_size[1]+1) << ","
            << (dfsm_size[2]+1) << ","
            << (dfsm_size[0]) * (dfsm_size[1]+1) << ","
            << avgWMethodAddStates << ","
            << avgWpMethodAddStates << ","
            << avgDMethodAddStates << ","
            << avgADSMethodAddStates << ","
            << avgHDMethodAddStates << ","
            << avgHADSMethodAddStates << endl;
    }
    out.close();
    cout << "Finished!" << endl << endl;
}

void evaluateTestCaseLength() {
    cout << " Start Test Case Length Evaluation..." << endl;

    int numDfsm = 50;

    vector<vector<int>> dfsm_sizes = {{10,1,1}, //20
                                      {10,2,2}, //30
                                      {10,3,3}, //40
                                      {20,2,2}, //60
                                      {20,3,3}, //80
                                      {20,4,4}, //100
                                      {20,5,5}, //120
                                      {30,4,4}, //150
                                      {30,5,5}, //180
                                      {30,6,6}, //210
                                      {50,4,4}, //250
                                      {50,6,6}, //350
                                      {50,7,7}, //400
                                      {50,8,8}, //450
                                      {70,7,7}, //560
                                      {70,8,8}, //630
                                      {70,9,9}, //700
                                      {70,11,11}, //840
                                      {80,13,13}, //1120
                                      {100,9,9}, //1000
                                      {100,11,11}, //1200
                                      {100,12,12}, //1300
    };

    std::cout << std::fixed << std::showpoint;
    std::cout << std::setprecision(2);

    ofstream out("testcase_average_length.csv");
    out << "states  ,"
        << "inputs  ,"
        << "outputs ,"
        << "transitions ,"
        << "W-Method,"
        << "Wp-Method,"
        << "D-Method,"
        << "D-Method(ADS),"
        << "Hierons D-Method,"
        << "Hierons D-Method(ADS)" << endl;

    for(auto& dfsm_size:dfsm_sizes) {
        cout << endl << "Evaluate Testcase length for DFSM with: " << endl
             << "\t -states -> " << to_string(dfsm_size[0]) << endl
             << "\t -inputs -> " << to_string(dfsm_size[1]) << endl
             << "\t -outputs -> " << to_string(dfsm_size[2]) << endl;

        double avgWMethSize = 0,
                avgWpMethSize = 0,
                avgDMethSize = 0,
                avgADSMethSize = 0,
                avgHDMethSize = 0,
                avgHADSMethSize = 0;

        cout << "Evaluate Testcase length for W-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->wMethodOnMinimisedDfsm(0);

            int avgTCLength = 0;
            for(auto& tc:*ts.getIOLists()) {
                avgTCLength += tc.size();
            }
            avgTCLength = (double) avgTCLength / (double) ts.getIOLists()->size();

            avgWMethSize += avgTCLength;
        }
        avgWMethSize = (double) avgWMethSize/(double) numDfsm;

        cout << "Evaluate Testcase length for Wp-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0]) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->wpMethodOnMinimisedDfsm(0);

            int avgTCLength = 0;
            for(auto& tc:*ts.getIOLists()) {
                //cout << "tc length -> " << tc.size() << endl;
                avgTCLength += tc.size();
            }
            avgTCLength = (double) avgTCLength / (double) ts.getIOLists()->size();

            avgWpMethSize += avgTCLength;
        }
        avgWpMethSize = (double) avgWpMethSize/(double) numDfsm;

        cout << "Evaluate Testcase length for D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->dMethodOnMinimisedDfsm(0,false);

            int avgTCLength = 0;
            for(auto& tc:*ts.getIOLists()) {
                avgTCLength += tc.size();
            }
            avgTCLength = (double) avgTCLength / (double) ts.getIOLists()->size();

            avgDMethSize += avgTCLength;
        }
        avgDMethSize = (double) avgDMethSize/(double) numDfsm;

        cout << "Evaluate Testcase length for D-Method(ADS):" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->dMethodOnMinimisedDfsm(0,true);

            int avgTCLength = 0;
            for(auto& tc:*ts.getIOLists()) {
                avgTCLength += tc.size();
            }
            avgTCLength = (double) avgTCLength / (double) ts.getIOLists()->size();

            avgADSMethSize += avgTCLength;
        }
        avgADSMethSize = (double) avgADSMethSize/(double) numDfsm;

        cout << "Evaluate Testcase length for Hierons D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->hieronsDMethodOnMinimisedDfsm(false);

            int avgTCLength = 0;
            for(auto& tc:*ts.getIOLists()) {
                avgTCLength += tc.size();
            }
            avgTCLength = (double) avgTCLength / (double) ts.getIOLists()->size();

            avgHDMethSize += avgTCLength;
        }
        avgHDMethSize = (double) avgHDMethSize/(double) numDfsm;

        cout << "Evaluate Testcase length for Hierons D-Method:" << endl;
        for(int i=0;i<numDfsm;++i) {
            shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(dfsm_size[0], dfsm_size[1], dfsm_size[2]);
            //create random minimised dfsm with certain number of states, inputs and outputs
            shared_ptr<Dfsm> dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                                        dfsm_size[2], pl)->minimise());
            auto ds = dfsm->createDistinguishingSequence();
            //generate random dfsm that are already minimised
            while(dfsm->size() < dfsm_size[0] || ds.empty()) {
                dfsm = make_shared<Dfsm>(make_shared<Dfsm>("Dfsm", dfsm_size[0], dfsm_size[1],
                                                           dfsm_size[2], pl)->minimise());
                ds = dfsm->createDistinguishingSequence();
            }
            cout << "Created next random minimised DFSM(" << i << ")..." << endl;

            auto ts = dfsm->hieronsDMethodOnMinimisedDfsm(true);

            int avgTCLength = 0;
            for(auto& tc:*ts.getIOLists()) {
                avgTCLength += tc.size();
            }
            avgTCLength = (double) avgTCLength / (double) ts.getIOLists()->size();

            avgHADSMethSize += avgTCLength;
        }
        avgHADSMethSize = (double) avgHADSMethSize/(double) numDfsm;

        out << dfsm_size[0] << ","
            << (dfsm_size[1]+1) << ","
            << (dfsm_size[2]+1) << ","
            << (dfsm_size[0]) * (dfsm_size[1]+1) << ","
            << avgWMethSize << ","
            << avgWpMethSize << ","
            << avgDMethSize << ","
            << avgADSMethSize << ","
            << avgHDMethSize << ","
            << avgHADSMethSize << endl;
    }
    out.close();
    cout << "Finished!" << endl << endl;
}

void evaluateTCPDfsm()
{
    int numMutants = 100,
        numAddStates = 5;

    cout << "Starting Evaluation for tcp.fsm..." << endl;
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>("../../../resources/tcpIn.txt",
                                                                  "../../../resources/tcpOut.txt",
                                                                  "../../../resources/tcpState.txt");
    shared_ptr<Dfsm> dfsm = make_shared<Dfsm>("../../../resources/tcp.fsm",pl,"tcp");
    auto minDfsm = dfsm->minimise();

    cout << "orig. size -> " << dfsm->size() << endl;
    cout << "min. size -> " << minDfsm.size() << endl;

    auto dTs = dfsm->dMethodOnMinimisedDfsm(0,false);
    auto adsTs = dfsm->dMethodOnMinimisedDfsm(0,true);
    auto hdTs = dfsm->hieronsDMethodOnMinimisedDfsm(false);
    auto hadsTs = dfsm->hieronsDMethodOnMinimisedDfsm(true);

    bool dsExists = dTs.size() > 0;
    bool adsExists = adsTs.size() > 0;

    double avgLengthSidPds = 0,
            avgLengthSidAds = 0,
            avgLengthHierPds = 0,
            avgLengthHierAds = 0,
            avgLengthW = 0,
            avgLengthWp = 0,
            avgLengthH = 0;

    cout << "DS exists -> " << dsExists << endl;
    if(dsExists) {
        cout << "DS length -> " << dfsm->createDistinguishingSequence().size() << endl;
    }
    cout << "ADS exists -> " << adsExists << endl;
    if(adsExists) {
        auto hsi = dfsm->createAdaptiveDistinguishingSequence()->getHsi();
        int maxDepth = hsi->begin()->size();
        for(auto h: *hsi) {
            cout << "lel -> " << h.size() << endl;
            maxDepth = maxDepth >= h.size()?maxDepth:h.size();
        }
        cout << "ADS depth -> " << maxDepth << endl;
    }

    vector<IOTrace> sidPdsTs;
    for(vector<int> v: *dTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        sidPdsTs.push_back(io);
        avgLengthSidPds += v.size();
    }
    avgLengthSidPds = sidPdsTs.size()?avgLengthSidPds / (double) sidPdsTs.size():0;

    vector<IOTrace> sidAdsTs;
    for(vector<int> v: *adsTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        sidAdsTs.push_back(io);
        avgLengthSidAds += v.size();
    }
    avgLengthSidAds = sidAdsTs.size()?avgLengthSidAds / (double) sidAdsTs.size():0;


    vector<IOTrace> hierPdsTs;
    for(vector<int> v: *hdTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        hierPdsTs.push_back(io);
        avgLengthHierPds+= v.size();
    }
    avgLengthHierPds = hierPdsTs.size()?avgLengthHierPds / (double) hierPdsTs.size():0;

    vector<IOTrace> hierAdsTs;
    for(vector<int> v: *hadsTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        hierAdsTs.push_back(io);
        avgLengthHierAds+= v.size();
    }
    avgLengthHierAds = hierAdsTs.size()?avgLengthHierAds / (double) hierAdsTs.size():0;

    auto w =  dfsm->wMethodOnMinimisedDfsm(0);
    vector<IOTrace> wTs;
    for(vector<int> v: *w.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        wTs.push_back(io);
        avgLengthW += v.size();
    }
    avgLengthW = wTs.size()?avgLengthW/ (double) wTs.size():0;

    auto wp =  dfsm->wpMethodOnMinimisedDfsm(0);
    vector<IOTrace> wpTs;
    for(vector<int> v: *wp.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        wpTs.push_back(io);
        avgLengthWp += v.size();
    }
    avgLengthWp = wpTs.size()?avgLengthWp/ (double) wpTs.size():0;

    auto h =  dfsm->hMethodOnMinimisedDfsm(0);
    vector<IOTrace> hTs;
    for(vector<int> v: *h.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        hTs.push_back(io);
        avgLengthH += v.size();
    }
    avgLengthH = wpTs.size()?avgLengthH/ (double) hTs.size():0;

    cout << "Created testsuites for all Methods..." << endl;

    float sidPds_fc = 0,
            sidAds_fc = 0,
            hierPds_fc = 0,
            hierAds_fc = 0;

    int num_nonequal_mut = 0,
            num_not_passing_sidPds = 0,
            num_not_passing_sidAds = 0,
            num_not_passing_hierPds = 0,
            num_not_passing_hierAds = 0;

    for(int j=0;j<numMutants;++j) {
        unsigned int numOutputFaults = rand() % (dfsm->getMaxNodes()/3),
                numTransitionFaults = rand() % (dfsm->getMaxNodes()/3);
        if(random_bool_with_prob(0.4)) {
            numOutputFaults = 0;
            numTransitionFaults = 0;
        }
        shared_ptr<Dfsm> mutant = dfsm->createMutant("Mutant",numOutputFaults,numTransitionFaults);

        bool result = dfsm->equivalenceCheck(*mutant);
        if (!result) num_nonequal_mut++;

        result = true;
        for (IOTrace io: sidPdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_sidPds++;

        result = true;
        for (IOTrace io: sidAdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_sidAds++;

        result = true;
        for (IOTrace io: hierPdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_hierPds++;

        result = true;
        for (IOTrace io: hierAdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_hierAds++;

    }

    sidPds_fc = (num_nonequal_mut>0?(float)num_not_passing_sidPds/(float)num_nonequal_mut*100:
             ((num_not_passing_sidPds == 0)?100:0));
    sidAds_fc = (num_nonequal_mut>0?(float)num_not_passing_sidAds/(float)num_nonequal_mut*100:
             ((num_not_passing_sidAds == 0)?100:0));
    hierPds_fc = (num_nonequal_mut>0?(float)num_not_passing_hierPds/(float)num_nonequal_mut*100:
              ((num_not_passing_hierPds == 0)?100:0));
    hierAds_fc = (num_nonequal_mut>0?(float)num_not_passing_hierAds/(float)num_nonequal_mut*100:
              ((num_not_passing_hierAds == 0)?100:0));

    cout << endl
            << "Number of unequal mutants -> " << num_nonequal_mut << endl
            << "Not passing D-Method(PDS) -> " << num_not_passing_sidPds << endl
            << "Not passing D-Method(ADS) -> " << num_not_passing_sidAds << endl
            << "Not passing Hierons D-Method(PDS) -> " << num_not_passing_hierPds << endl
            << "Not passing Hierons D-Method(ADS) -> " << num_not_passing_hierAds << endl;

    cout << "D-Method(PDS) Fault Coverage -> " << sidPds_fc << endl
            << "D-Method(ADS) Fault Coverage -> " << sidAds_fc << endl
            << "Hierons D-Method(PDS) Fault Coverage -> " << hierPds_fc << endl
            << "Hierons D-Method(ADS) Fault Coverage -> " << hierAds_fc << endl << endl;

    cout << endl
            << "W-Method size -> " << w.getFlatSize() << endl
            << "Wp-Method size -> " << wp.getFlatSize() << endl
            << "H-Method size -> " << h.getFlatSize() << endl
            << "D-Method size -> " << dTs.getFlatSize() << endl
            << "D-Method(ADS) size -> " << adsTs.getFlatSize() << endl
            << "H.D-Method size -> " << hdTs.getFlatSize() << endl
            << "H.D-Method(ADS) size -> " << hadsTs.getFlatSize() << endl;

    cout << endl
         << "W-Method avg length -> " << avgLengthW << endl
         << "Wp-Method avg length -> " << avgLengthWp << endl
         << "H-Method avg length -> " << avgLengthH << endl
         << "D-Method avg length -> " << avgLengthSidPds << endl
         << "D-Method(ADS) avg length -> " << avgLengthSidAds << endl
         << "H.D-Method avg length -> " << avgLengthHierPds << endl
         << "H.D-Method(ADS) avg length -> " << avgLengthHierAds << endl;

    //Calculate fault coverage outside fault domain for a limited number of additional states
    ofstream out("tcp_fc_add_states.csv");
    out << "No. Add. States,"
        << "W-Method,"
        << "Wp-Method,"
        << "H-Method,"
        << "D-Method,"
        << "D-Method(ADS),"
        << "Hierons D-Method,"
        << "Hierons D-Method(ADS)" << endl;


    for(int i=1;i<=numAddStates;++i) {
        sidPds_fc = 0;
        sidAds_fc = 0;
        hierPds_fc = 0;
        hierAds_fc = 0;
        float w_fc = 0,
            wp_fc = 0,
            h_fc = 0;


        int num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: sidPdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        sidPds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
              ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: sidAdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        sidAds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: hierPdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        hierPds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: hierAdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        hierAds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: wTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        w_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: wpTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        wp_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: hTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        h_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        out << i << ","
            << w_fc << ","
            << wp_fc << ","
            << h_fc << ","
            << sidPds_fc << ","
            << sidAds_fc << ","
            << hierPds_fc << ","
            << hierAds_fc << endl;
    }

    out.close();

    cout << "Finished with SUCCESS!" << endl << endl;
}

void evaluateLee94Dfsm()
{
    int numMutants = 100,
            numAddStates = 5;

    cout << "Starting Evaluation for lee94_no_pds.fsm..." << endl;
    shared_ptr<FsmPresentationLayer> pl = createPresentationLayer(1,6,1);
    shared_ptr<Dfsm> dfsm = make_shared<Dfsm>("../../../resources/lee94_no_pds.fsm",pl,"lee94_no_pds");
    auto minDfsm = dfsm->minimise();

    cout << "orig. size -> " << dfsm->size() << endl;
    cout << "min. size -> " << minDfsm.size() << endl;

    auto dTs = dfsm->dMethodOnMinimisedDfsm(0,false);
    auto adsTs = dfsm->dMethodOnMinimisedDfsm(0,true);
    auto hdTs = dfsm->hieronsDMethodOnMinimisedDfsm(false);
    auto hadsTs = dfsm->hieronsDMethodOnMinimisedDfsm(true);

    bool dsExists = dTs.size() > 0;
    bool adsExists = adsTs.size() > 0;

    double avgLengthSidPds = 0,
        avgLengthSidAds = 0,
            avgLengthHierPds = 0,
            avgLengthHierAds = 0,
            avgLengthW = 0,
            avgLengthWp = 0,
            avgLengthH = 0;

    cout << "DS exists -> " << dsExists << endl;
    if(dsExists) {
        cout << "DS length -> " << dfsm->createDistinguishingSequence().size() << endl;
    }
    cout << "ADS exists -> " << adsExists << endl;
    if(adsExists) {
        auto hsi = dfsm->createAdaptiveDistinguishingSequence()->getHsi();
        int maxDepth = hsi->begin()->size();
        for(auto h: *hsi) {
            cout << "lel -> " << h.size() << endl;
            maxDepth = maxDepth >= h.size()?maxDepth:h.size();
        }
        cout << "ADS depth -> " << maxDepth << endl;
    }

    vector<IOTrace> sidPdsTs;
    for(vector<int> v: *dTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        sidPdsTs.push_back(io);
        avgLengthSidPds += v.size();
    }
    avgLengthSidPds = sidPdsTs.size()?avgLengthSidPds / (double) sidPdsTs.size():0;

    vector<IOTrace> sidAdsTs;
    for(vector<int> v: *adsTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        sidAdsTs.push_back(io);
        avgLengthSidAds += v.size();
    }
    avgLengthSidAds = sidAdsTs.size()?avgLengthSidAds / (double) sidAdsTs.size():0;


    vector<IOTrace> hierPdsTs;
    for(vector<int> v: *hdTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        hierPdsTs.push_back(io);
        avgLengthHierPds+= v.size();
    }
    avgLengthHierPds = hierPdsTs.size()?avgLengthHierPds / (double) hierPdsTs.size():0;

    vector<IOTrace> hierAdsTs;
    for(vector<int> v: *hadsTs.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        hierAdsTs.push_back(io);
        avgLengthHierAds+= v.size();
    }
    avgLengthHierAds = hierAdsTs.size()?avgLengthHierAds / (double) hierAdsTs.size():0;

    auto w =  dfsm->wMethodOnMinimisedDfsm(0);
    vector<IOTrace> wTs;
    for(vector<int> v: *w.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        wTs.push_back(io);
        avgLengthW += v.size();
    }
    avgLengthW = wTs.size()?avgLengthW/ (double) wTs.size():0;

    auto wp =  dfsm->wpMethodOnMinimisedDfsm(0);
    vector<IOTrace> wpTs;
    for(vector<int> v: *wp.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        wpTs.push_back(io);
        avgLengthWp += v.size();
    }
    avgLengthWp = wpTs.size()?avgLengthWp/ (double) wpTs.size():0;

    auto h =  dfsm->hMethodOnMinimisedDfsm(0);
    vector<IOTrace> hTs;
    for(vector<int> v: *h.getIOLists()) {
        InputTrace i(v,pl);
        IOTrace io = dfsm->applyDet(i);
        hTs.push_back(io);
        avgLengthH += v.size();
    }
    avgLengthH = wpTs.size()?avgLengthH/ (double) hTs.size():0;

    cout << "Created testsuites for all Methods..." << endl;

    float sidPds_fc = 0,
            sidAds_fc = 0,
            hierPds_fc = 0,
            hierAds_fc = 0;

    int num_nonequal_mut = 0,
            num_not_passing_sidPds = 0,
            num_not_passing_sidAds = 0,
            num_not_passing_hierPds = 0,
            num_not_passing_hierAds = 0;

    for(int j=0;j<numMutants;++j) {
        unsigned int numOutputFaults = rand() % (dfsm->getMaxNodes()/3),
                numTransitionFaults = rand() % (dfsm->getMaxNodes()/3);
        if(random_bool_with_prob(0.4)) {
            numOutputFaults = 0;
            numTransitionFaults = 0;
        }
        shared_ptr<Dfsm> mutant = dfsm->createMutant("Mutant",numOutputFaults,numTransitionFaults);

        bool result = dfsm->equivalenceCheck(*mutant);
        if (!result) num_nonequal_mut++;

        result = true;
        for (IOTrace io: sidPdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_sidPds++;

        result = true;
        for (IOTrace io: sidAdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_sidAds++;

        result = true;
        for (IOTrace io: hierPdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_hierPds++;

        result = true;
        for (IOTrace io: hierAdsTs) {
            result &= mutant->pass(io);
        }
        if (!result) num_not_passing_hierAds++;

    }

    sidPds_fc = (num_nonequal_mut>0?(float)num_not_passing_sidPds/(float)num_nonequal_mut*100:
                 ((num_not_passing_sidPds == 0)?100:0));
    sidAds_fc = (num_nonequal_mut>0?(float)num_not_passing_sidAds/(float)num_nonequal_mut*100:
                 ((num_not_passing_sidAds == 0)?100:0));
    hierPds_fc = (num_nonequal_mut>0?(float)num_not_passing_hierPds/(float)num_nonequal_mut*100:
                  ((num_not_passing_hierPds == 0)?100:0));
    hierAds_fc = (num_nonequal_mut>0?(float)num_not_passing_hierAds/(float)num_nonequal_mut*100:
                  ((num_not_passing_hierAds == 0)?100:0));

    cout << endl
         << "Number of unequal mutants -> " << num_nonequal_mut << endl
         << "Not passing D-Method(PDS) -> " << num_not_passing_sidPds << endl
         << "Not passing D-Method(ADS) -> " << num_not_passing_sidAds << endl
         << "Not passing Hierons D-Method(PDS) -> " << num_not_passing_hierPds << endl
         << "Not passing Hierons D-Method(ADS) -> " << num_not_passing_hierAds << endl;

    cout << "D-Method(PDS) Fault Coverage -> " << sidPds_fc << endl
         << "D-Method(ADS) Fault Coverage -> " << sidAds_fc << endl
         << "Hierons D-Method(PDS) Fault Coverage -> " << hierPds_fc << endl
         << "Hierons D-Method(ADS) Fault Coverage -> " << hierAds_fc << endl << endl;

    cout << endl
         << "W-Method size -> " << w.getFlatSize() << endl
         << "Wp-Method size -> " << wp.getFlatSize() << endl
         << "H-Method size -> " << h.getFlatSize() << endl
         << "D-Method size -> " << dTs.getFlatSize() << endl
         << "D-Method(ADS) size -> " << adsTs.getFlatSize() << endl
         << "H.D-Method size -> " << hdTs.getFlatSize() << endl
         << "H.D-Method(ADS) size -> " << hadsTs.getFlatSize() << endl;

    cout << endl
         << "W-Method avg length -> " << avgLengthW << endl
         << "Wp-Method avg length -> " << avgLengthWp << endl
         << "H-Method avg length -> " << avgLengthH << endl
         << "D-Method avg length -> " << avgLengthSidPds << endl
         << "D-Method(ADS) avg length -> " << avgLengthSidAds << endl
         << "H.D-Method avg length -> " << avgLengthHierPds << endl
         << "H.D-Method(ADS) avg length -> " << avgLengthHierAds << endl;

    //Calculate fault coverage outside fault domain for a limited number of additional states
    ofstream out("lee94_fc_add_states.csv");
    out << "No. Add. States,"
        << "W-Method,"
        << "Wp-Method,"
        << "H-Method,"
        << "D-Method,"
        << "D-Method(ADS),"
        << "Hierons D-Method,"
        << "Hierons D-Method(ADS)" << endl;

    for(int i=1;i<=numAddStates;++i) {
        sidPds_fc = 0;
        sidAds_fc = 0;
        hierPds_fc = 0;
        hierAds_fc = 0;
        float w_fc = 0,
                wp_fc = 0,
                h_fc = 0;


        int num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: sidPdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        sidPds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: sidAdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        sidAds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                     ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: hierPdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        hierPds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                      ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: hierAdsTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        hierAds_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                      ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: wTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        w_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: wpTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        wp_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                 ((num_not_pass == 0)?100:0));

        num_not_pass = 0;
        num_nonequal_mut = 0;
        for(int j=0;j<numMutants;++j) {
            shared_ptr<Dfsm> mutant = make_shared<Dfsm>(
                    dfsm->createMutant("Mutant",0,0,i)->minimise());

            while(mutant->size() < (dfsm->size() + i)) {
                mutant = make_shared<Dfsm>(
                        dfsm->createMutant("Mutant",0,0,i)->minimise());
            }

            bool result = dfsm->equivalenceCheck(*mutant);
            if (!result) num_nonequal_mut++;

            result = true;
            for (IOTrace io: hTs) {
                result &= mutant->pass(io);
            }
            if (!result) num_not_pass++;
        }
        h_fc = (num_nonequal_mut>0?(float)num_not_pass/(float)num_nonequal_mut*100:
                ((num_not_pass == 0)?100:0));

        out << i << ","
            << w_fc << ","
            << wp_fc << ","
            << h_fc << ","
            << sidPds_fc << ","
            << sidAds_fc << ","
            << hierPds_fc << ","
            << hierAds_fc << endl;
    }

    out.close();

    cout << "Finished with SUCCESS!" << endl << endl;
}

int main(int argc, char* argv[])
{

    srand(getRandomSeed());

    //Logging
    //LogCoordinator& logger = LogCoordinator::getStandardLogger();
    //logger.setDefaultStream(cout);

    evaluateLee94Dfsm();
    evaluateTCPDfsm();
    evaluateDMethodsApplicability();
    evaluateTestCaseLength();
    evaluateFCOutsideFaultDomain();
    evaluateDMethodsFaultCoverage();
    evaluateTestSuiteSizes();
    testRandomPdsAndAds(6,2,2);
    testLeeAds();

}