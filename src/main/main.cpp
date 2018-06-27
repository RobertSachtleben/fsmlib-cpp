/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <stdlib.h>
#include <interface/FsmPresentationLayer.h>
#include <fsm/Dfsm.h>
#include <fsm/Fsm.h>
#include <fsm/FsmNode.h>
#include <fsm/IOTrace.h>
#include <fsm/FsmPrintVisitor.h>
#include <fsm/FsmSimVisitor.h>
#include <fsm/FsmOraVisitor.h>
#include <trees/IOListContainer.h>
#include <trees/OutputTree.h>
#include <trees/TestSuite.h>
#include "json/json.h"


using namespace std;
using namespace Json;

void assertInconclusive(string tc, string comment = "") {
    
    string sVerdict("INCONCLUSIVE");
    cout << sVerdict << ": " << tc << " : " << comment <<  endl;
    
}

void assert(string tc, bool verdict, string comment = "") {
    
    string sVerdict = (verdict) ? "PASS" : "FAIL";
    cout << sVerdict << ": " << tc
    << " : "
    << comment <<  endl;
    
}

void test1() {
    
    cout << "TC-DFSM-0001 Show that Dfsm.applyDet() deals correctly with incomplete DFSMs "
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    Dfsm d("../../../resources/TC-DFSM-0001.fsm",pl,"m1");
    d.toDot("TC-DFSM-0001");
    
    vector<int> inp;
    inp.push_back(1);
    inp.push_back(0);
    inp.push_back(0);
    inp.push_back(0);
    inp.push_back(1);
    
    
    InputTrace i(inp,pl);
    
    cout << "InputTrace = " << i << endl;
    
    
    IOTrace t = d.applyDet(i);
    
    cout << "IOTrace t = " << t << endl;
    
    vector<int> vIn = t.getInputTrace().get();
    vector<int> vOut = t.getOutputTrace().get();
    assert("TC-DFSM-0001",vIn.size() == 4
           and vOut.size() == 4
           and vOut[0] == 2
           and vOut[1] == 0
           and vOut[2] == 2
           and vOut[3] == 2,
           "For input trace 1.0.0.0.1, the output trace is 2.0.2.2");
    
    
    inp.insert(inp.begin(),9);
    InputTrace j(inp,pl);
    IOTrace u = d.applyDet(j);
    cout << "IOTrace u = " << u << endl;
    assert("TC-DFSM-0001",
           u.getOutputTrace().get().size() == 0 and
           u.getInputTrace().get().size() == 0,
           "For input trace 9, the output trace is empty.");
    
}


void test2() {
    
    cout << "TC-FSM-0001 Show that the copy constructor produces a deep copy of an FSM generated at random "
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    shared_ptr<Fsm> f1 = Fsm::createRandomFsm("f1",3,5,10,pl);
    
    shared_ptr<Fsm> f2 = make_shared<Fsm>(*f1);
    
    f1->toDot("f1");
    f2->toDot("f1Copy");
    
    // Check using diff, that the dot-files of both FSMs
    // are identical
    assert("TC-FSM-0001", 0 == system("diff f1.dot f1Copy.dot"),
           "dot-files of original and copied FSM are identical");
    
    cout << "Show that original FSM and deep copy are equivalent, "
    << endl << "using the WpMethod";
    
    Fsm f1Obs = f1->transformToObservableFSM();
    Fsm f1Min = f1Obs.minimise();
    
    Fsm f2Obs = f2->transformToObservableFSM();
    Fsm f2Min = f2Obs.minimise();
    
    int m = (f2Min.getMaxNodes() > f1Min.getMaxNodes() ) ?
    (f2Min.getMaxNodes() - f1Min.getMaxNodes()) : 0;
    IOListContainer iolc = f1Min.wMethod(m);
    
    TestSuite t1 = f1Min.createTestSuite(iolc);
    TestSuite t2 = f2Min.createTestSuite(iolc);
    
    assert("TC-FSM-0001",
           t2.isEquivalentTo(t1),
           "Original FSM and its deep copy pass the same W-Method test suite");
    
    
    
}

void test3() {
    
    cout << "TC-FSM-0002 Show that createMutant() injects a fault into the original FSM" << endl;
    
    
    for ( size_t i = 0; i < 4; i++ ) {
        shared_ptr<FsmPresentationLayer> pl =
        make_shared<FsmPresentationLayer>();
        shared_ptr<Fsm> fsm = Fsm::createRandomFsm("F",5,5,8,pl,(unsigned)i);
        fsm->toDot("F");
        
        shared_ptr<Fsm> fsmMutant = fsm->createMutant("F_M",1,0);
        fsmMutant->toDot("FMutant");
        
        Fsm fsmMin = fsm->minimise();
        fsmMin.toDot("FM");
        
        Fsm fsmMutantMin = fsmMutant->minimise();
        
        unsigned int m = 0;
        if ( fsmMutantMin.getMaxNodes() > fsmMin.getMaxNodes() ) {
            m = fsmMutantMin.getMaxNodes() - fsmMin.getMaxNodes();
        }
        
        cout << "Call W-Method - additional states (m) = " << m << endl;
        
        IOListContainer iolc1 = fsmMin.wMethodOnMinimisedFsm(m);
        
        cout << "TS SIZE (W-Method): " << iolc1.size() << endl;
        
        if ( iolc1.size() > 1000) {
            cout << "Skip this test case, since size is too big" << endl;
            continue;
        }
        
        TestSuite t1 = fsmMin.createTestSuite(iolc1);
        TestSuite t2 = fsmMutantMin.createTestSuite(iolc1);
        
        assert("TC-FSM-0002", not t2.isEquivalentTo(t1),
               "Original FSM and mutant do not produce the same test suite results - tests are created by W-Method");
        
        IOListContainer iolc2 = fsmMin.wpMethod(m);
        
        cout << "TS SIZE (Wp-Method): " << iolc2.size() << endl;
        
        if ( iolc2.size() > iolc1.size() ) {
            
            ofstream outFile("fsmMin.fsm");
            fsmMin.dumpFsm(outFile);
            outFile.close();
             
            exit(1);
        }

        
        TestSuite t1wp = fsmMin.createTestSuite(iolc2);
        TestSuite t2wp = fsmMutantMin.createTestSuite(iolc2);
        
        assert("TC-FSM-0002",
               not t2wp.isEquivalentTo(t1wp),
               "Original FSM and mutant do not produce the same test suite results - tests are created by Wp-Method");
        
        assert("TC-FSM-0002",
               t1wp.size() <= t1.size(),
               "Wp-Method test suite size less or equal to W-Method size");
        
        if ( t1wp.size() > t1.size() ) {
            cout << "Test Suite Size (W-Method): " << t1.size()
            << endl << "Test Suite Size (Wp-Method): " << t1wp.size() << endl;
            cout << endl << "W-Method " << endl << iolc1 << endl;
            exit(1);
        }
        
        
    }
    
    
}


void test4() {
    
    cout << "TC-FSM-0004 Check correctness of state cover" << endl;
    
    const bool markAsVisited = true;
    bool havePassed = true;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    for (size_t i = 0; i < 2000; i++) {
        
        // Create a random FSM
        std::shared_ptr<Fsm> f = Fsm::createRandomFsm("F",5,5,10,pl,(unsigned)i);
        std::shared_ptr<Tree> sc = f->getStateCover();
        
        if ( sc->size() != (size_t)f->getMaxNodes() + 1 ) {
            cout << "Size of state cover: " << sc->size()
            << " Number of states in FSM: " << f->getMaxNodes() + 1 << endl;
            assert("TC-FSM-0004",
                   sc->size() <= (size_t)f->getMaxNodes() + 1,
                   "Size of state cover must be less or equal than number of FSM states");
        }
        
        
        IOListContainer c = sc->getTestCases();
        std::shared_ptr<std::vector<std::vector<int>>> iols = c.getIOLists();
        
        for ( auto inLst : *iols ) {
            auto iTr = make_shared<InputTrace>(inLst,pl);
            f->apply(*iTr,true);
        }
        
        for ( std::shared_ptr<FsmNode> n : f->getNodes() ) {
            if ( not n->hasBeenVisited() ) {
                havePassed = false;
                assert("TC-FSM-0004",
                       n->hasBeenVisited(),
                       "State cover failed to visit node " + n->getName());
                
                f->toDot("FailedStateCoverFSM");
                
                filebuf fb;
                fb.open ("FailedStateCover.dot",std::ios::out);
                ostream os(&fb);
                sc->toDot(os);
                fb.close();
                
                int iCtr = 0;
                for ( auto inLst : *iols ) {
                    ostringstream oss;
                    oss << iCtr++;
                    auto iTr = make_shared<InputTrace>(inLst,pl);
                    filebuf fbot;
                    OutputTree ot = f->apply(*iTr,markAsVisited);
                    fbot.open ("FailedStateCover" + oss.str() + ".dot",
                               std::ios::out);
                    ostream osdot(&fbot);
                    sc->toDot(osdot);
                    fbot.close();
                }
                
                exit(1);
                
            }
        }
        
    }
    
    if ( havePassed ) {
        assert("TC-FSM-0004",
               true,
               "State cover reaches all states");
    }
    else {
        exit(0);
    }
    
    
}

void test5() {
    
    cout << "TC-FSM-0005 Check correctness of input " <<
    "equivalence classes" << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>();
    
    
    shared_ptr<Fsm> fsm =
    make_shared<Fsm>("../../../resources/TC-FSM-0005.fsm",pl,"F");
    fsm->toDot("TC-FSM-0005");
    
    vector< std::unordered_set<int> > v = fsm->getEquivalentInputs();
    
    for ( size_t s = 0; s < v.size(); s++ ) {
        cout << s << ": { ";
        bool isFirst = true;
        for ( auto x : v[s] ) {
            if ( isFirst ) {
                isFirst= false;
            }
            else   {
                cout << ", ";
            }
            cout << x;
        }
        cout << " }" << endl;
    }
    
    assert("TC-FSM-0005",
           v.size() == 3,
           "For TC-FSM-0005.fsm, there are 3 classes of equivalent inputs.");
    
    assert("TC-FSM-0005",
           v[0].size() == 1 and v[0].find(0) != v[0].end(),
           "Class 0 only contains input 0.");
    
    assert("TC-FSM-0005",
           v[1].size() == 1 and v[1].find(1) != v[1].end(),
           "Class 1 only contains input 1.");
    
    assert("TC-FSM-0005",
           v[2].size() == 2 and
           v[2].find(2) != v[2].end() and
           v[2].find(3) != v[2].end(),
           "Class 2 contains inputs 2 and 3.");
    
    
    // Check FSM without any equivalent inputs
    fsm = make_shared<Fsm>("../../../resources/fsmGillA7.fsm",pl,"F");
    fsm->toDot("fsmGillA7");
    v = fsm->getEquivalentInputs();
    
    assert("TC-FSM-0005",
           v.size() == 3,
           "For fsmGillA7, there are 3 input classes.");
    
    bool ok = true;
    for ( size_t s=0; s < v.size() and ok; s++ ) {
        if ( v[s].size() != 1 or
            v[s].find((int)s) == v[s].end() ) {
            ok =false;
        }
    }
    
    assert("TC-FSM-0005",
           ok,
           "For fsmGillA7, class x just contains input x.");
    
}

void test6() {
    
    cout << "TC-FSM-0006 Check correctness of FSM Print Visitor " << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    Dfsm d("../../../resources/TC-DFSM-0001.fsm",pl,"m1");
    
    FsmPrintVisitor v;
    
    d.accept(v);
    
    cout << endl << endl;
    assertInconclusive("TC-FSM-0006",
                       "Output of print visitor has to be checked manually");
    
    
}

void test7() {
    
    //    cout << "TC-FSM-0007 Check correctness of FSM Simulation Visitor "
    //    << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                      "../../../resources/garageOut.txt",
                                      "../../../resources/garageState.txt");
    Dfsm d("../../../resources/garage.fsm",pl,"GC");
    d.toDot("GC");
    
    FsmSimVisitor v;
    
    d.accept(v);
    
    v.setFinalRun(true);
    d.accept(v);
    
    cout << endl << endl;
    //    assertInconclusive("TC-FSM-0007",
    //                       "Output of simulation visitor has to be checked manually");
}


void test8() {
    
    //    cout << "TC-FSM-0008 Check correctness of FSM Oracle Visitor "
    //    << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                      "../../../resources/garageOut.txt",
                                      "../../../resources/garageState.txt");
    Dfsm d("../../../resources/garage.fsm",pl,"GC");
    d.toDot("GC");
    
    FsmOraVisitor v;
    
    d.accept(v);
    
    v.setFinalRun(true);
    d.accept(v);
    
    cout << endl << endl;
    //    assertInconclusive("TC-FSM-0008",
    //                       "Output of oracle visitor has to be checked manually");
}

void test9() {
    
    cout << "TC-FSM-0009 Check correctness of method removeUnreachableNodes() "
         << endl;
    
    shared_ptr<Dfsm> d = nullptr;
    Reader jReader;
    Value root;
    stringstream document;
    ifstream inputFile("../../../resources/unreachable_gdc.fsm");
    document << inputFile.rdbuf();
    inputFile.close();
    
    if ( jReader.parse(document.str(),root) ) {
        d = make_shared<Dfsm>(root);
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    
    d->toDot("GU");
    
    size_t oldSize = d->size();
    
    vector<shared_ptr<FsmNode>> uNodes;
    if ( d->removeUnreachableNodes(uNodes) ) {
        
        d->toDot("G_all_reachable");
        
        for ( auto n : uNodes ) {
            cout << "Removed unreachable node: " << n->getName() << endl;
        }
        
        assert("TC-FSM-0009",
               uNodes.size() == 2 and (oldSize - d->size()) == 2,
               "All unreachable states have been removed");
    }
    else {
        assert("TC-FSM-0009",
               false,
               "Expected removeUnreachableNodes() to return FALSE");
    }
    
    
}


void test10() {
    
    cout << "TC-FSM-0010 Check correctness of Dfsm::minimise() "
    << endl;
    
    shared_ptr<Dfsm> d = nullptr;
    shared_ptr<FsmPresentationLayer> pl;
    Reader jReader;
    Value root;
    stringstream document;
    ifstream inputFile("../../../resources/unreachable_gdc.fsm");
    document << inputFile.rdbuf();
    inputFile.close();
    
    if ( jReader.parse(document.str(),root) ) {
        d = make_shared<Dfsm>(root);
        pl = d->getPresentationLayer();
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    
    Dfsm dMin = d->minimise();
    
    IOListContainer w = dMin.getCharacterisationSet();
    
    shared_ptr<std::vector<std::vector<int>>> inLst = w.getIOLists();
    
    bool allNodesDistinguished = true;
    for ( size_t n = 0; n < dMin.size(); n++ ) {
        
        shared_ptr<FsmNode> node1 = dMin.getNodes().at(n);
        
        for ( size_t m = n+1; m < dMin.size(); m++ ) {
            shared_ptr<FsmNode> node2 = dMin.getNodes().at(m);
            
            bool areDistinguished = false;
            
            for ( auto inputs : *inLst ) {
                
                shared_ptr<InputTrace> itr = make_shared<InputTrace>(inputs,pl);
                
                OutputTree o1 = node1->apply(*itr);
                OutputTree o2 = node2->apply(*itr);
                
                if ( o1 != o2 ) {
                    areDistinguished = true;
                    break;
                }
                
            }
            
            if ( not areDistinguished ) {
                
                assert("TC-FSM-0010",
                       false,
                       "All nodes of minimised DFSM must be distinguishable");
                cout << "Could not distinguish nodes "
                << node1->getName() << " and " << node2->getName() << endl;
                
                allNodesDistinguished = false;
            }
            
        }
        
    }
    
    if ( allNodesDistinguished ) {
        assert("TC-FSM-0010",
               true,
               "All nodes of minimised DFSM must be distinguishable");
    }
    
}


void test10b() {
    
    cout << "TC-FSM-1010 Check correctness of Dfsm::minimise() with DFSM huang201711"
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
        make_shared<FsmPresentationLayer>("../../../resources/huang201711in.txt",
                                          "../../../resources/huang201711out.txt",
                                          "../../../resources/huang201711state.txt");
    
    
    shared_ptr<Dfsm> d = make_shared<Dfsm>("../../../resources/huang201711.fsm",
                                           pl,
                                           "F");
    Dfsm dMin = d->minimise();
    
    IOListContainer w = dMin.getCharacterisationSet();
    
    shared_ptr<std::vector<std::vector<int>>> inLst = w.getIOLists();
    
    bool allNodesDistinguished = true;
    for ( size_t n = 0; n < dMin.size(); n++ ) {
        
        shared_ptr<FsmNode> node1 = dMin.getNodes().at(n);
        
        for ( size_t m = n+1; m < dMin.size(); m++ ) {
            shared_ptr<FsmNode> node2 = dMin.getNodes().at(m);
            
            bool areDistinguished = false;
            
            for ( auto inputs : *inLst ) {
                
                shared_ptr<InputTrace> itr = make_shared<InputTrace>(inputs,pl);
                
                OutputTree o1 = node1->apply(*itr);
                OutputTree o2 = node2->apply(*itr);
                
                if ( o1 != o2 ) {
                    areDistinguished = true;
                    break;
                }
                
            }
            
            if ( not areDistinguished ) {
                
                assert("TC-FSM-1010",
                       false,
                       "All nodes of minimised DFSM must be distinguishable");
                cout << "Could not distinguish nodes "
                << node1->getName() << " and " << node2->getName() << endl;
                
                allNodesDistinguished = false;
            }
            
        }
        
    }
    
    if ( allNodesDistinguished ) {
        assert("TC-FSM-1010",
               true,
               "All nodes of minimised DFSM must be distinguishable");
    }
    
}


void gdc_test1() {
    
    cout << "TC-GDC-0001 Check that the correct W-Method test suite "
    << endl << "is generated for the garage door controller example" << endl;

    
    shared_ptr<Dfsm> gdc =
    make_shared<Dfsm>("../../../resources/garage-door-controller.csv","GDC");
    
    shared_ptr<FsmPresentationLayer> pl = gdc->getPresentationLayer();
    
    gdc->toDot("GDC");
    gdc->toCsv("GDC");
    
    Dfsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
    IOListContainer iolc =
        gdc->wMethod(2);
    
    shared_ptr< TestSuite > testSuite =
        make_shared< TestSuite >();
    for ( auto inVec : *iolc.getIOLists() ) {
        shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
        testSuite->push_back(gdc->apply(*itrc));
    }
    
    int tcNum = 0;
    for ( auto iotrc : *testSuite ) {
        cout << "TC-" << ++tcNum << ": " << iotrc;
    }
    
    testSuite->save("testsuite.txt");
    
    assert("TC-GDC-0001",
            0 == system("diff testsuite.txt ../../../resources/gdc-testsuite.txt"),
           "Expected GDC test suite and generated suite are identical");
    
    
}




vector<IOTrace> runAgainstRefModel(shared_ptr<Dfsm> refModel,
                                   IOListContainer& c) {
    
    shared_ptr<FsmPresentationLayer> pl = refModel->getPresentationLayer();
    
    auto iolCnt = c.getIOLists();
    
    // Register test cases in IO Traces
    vector<IOTrace> iotrLst;

    for ( auto lst : *iolCnt ) {
        
        shared_ptr<InputTrace> itr = make_shared<InputTrace>(lst,pl);
        IOTrace iotr = refModel->applyDet(*itr);
        iotrLst.push_back(iotr);
        
    }
    
    return iotrLst;
    
}

void runAgainstMutant(shared_ptr<Dfsm> mutant, vector<IOTrace>& expected) {
    
    for ( auto io : expected ) {
        
        InputTrace i = io.getInputTrace();
        
        if ( not mutant->pass(io) ) {
            cout << "FAIL: expected " << io << endl
            << "     : observed " << mutant->applyDet(i) << endl;
        }
        else {
            cout << "PASS: " << i << endl;
        }
        
    }
    
}

void wVersusT() {
    
    shared_ptr<Dfsm> refModel = make_shared<Dfsm>("FSBRTSX.csv","FSBRTS");

//    IOListContainer wTestSuite0 = refModel->wMethod(0);
//    IOListContainer wTestSuite1 = refModel->wMethod(1);
//    IOListContainer wTestSuite2 = refModel->wMethod(2);
//    IOListContainer wTestSuite3 = refModel->wMethod(3);
//    
  IOListContainer wpTestSuite0 = refModel->wpMethod(0);
//    IOListContainer wpTestSuite1 = refModel->wpMethod(1);
//    IOListContainer wpTestSuite2 = refModel->wpMethod(2);
//    IOListContainer wpTestSuite3 = refModel->wpMethod(3);
    
    //    IOListContainer tTestSuite = refModel->tMethod();
    
//    vector<IOTrace> expectedResultsW0 = runAgainstRefModel(refModel, wTestSuite0);
//    vector<IOTrace> expectedResultsW1 = runAgainstRefModel(refModel, wTestSuite1);
//    vector<IOTrace> expectedResultsW2 = runAgainstRefModel(refModel, wTestSuite2);
//    vector<IOTrace> expectedResultsW3 = runAgainstRefModel(refModel, wTestSuite3);
    vector<IOTrace> expectedResultsWp0 = runAgainstRefModel(refModel, wpTestSuite0);
//    vector<IOTrace> expectedResultsWp1 = runAgainstRefModel(refModel, wpTestSuite1);
//    vector<IOTrace> expectedResultsWp2  = runAgainstRefModel(refModel, wpTestSuite2);
//    vector<IOTrace> expectedResultsWp3 = runAgainstRefModel(refModel, wpTestSuite3);
//    vector<IOTrace> expectedResultsT = runAgainstRefModel(refModel, tTestSuite);


    
    for ( int i = 0; i < 10; i++ ) {
        
        cout << "Mutant No. " << (i+1) << ": " << endl;
        
        shared_ptr<Dfsm> mutant =
            make_shared<Dfsm>("FSBRTSX.csv","FSBRTS");
        mutant->createAtRandom();
        
//        runAgainstMutant(mutant,expectedResultsW0);
//        runAgainstMutant(mutant,expectedResultsW1);
//        runAgainstMutant(mutant,expectedResultsW2);
//        runAgainstMutant(mutant,expectedResultsW3);
        runAgainstMutant(mutant,expectedResultsWp0);
//        runAgainstMutant(mutant,expectedResultsWp1);
//        runAgainstMutant(mutant,expectedResultsWp2);
//        runAgainstMutant(mutant,expectedResultsWp3);
//        runAgainstMutant(mutant,expectedResultsT);

        
    }
    
    
}

void test11() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                                                            "../../../resources/garageOut.txt",
                                                                            "../../../resources/garageState.txt");
    
    shared_ptr<Fsm> gdc = make_shared<Fsm>("../../../resources/garage.fsm",pl,"GDC");
    
    
    gdc->toDot("GDC");
    
    Fsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
}

void test12() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                                                            "../../../resources/garageOut.txt",
                                                                            "../../../resources/garageState.txt");
    
    shared_ptr<Dfsm> gdc = make_shared<Dfsm>("../../../resources/garage.fsm",pl,"GDC");
    
    
    gdc->toDot("GDC");
    
    Dfsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
}

void test13() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    shared_ptr<Dfsm> gdc = make_shared<Dfsm>("../../../resources/garage.fsm",pl,"GDC");
    
    
    gdc->toDot("GDC");
    
    Dfsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
}


void test14() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    shared_ptr<Fsm> fsm = make_shared<Fsm>("../../../resources/NN.fsm",pl,"NN");
    
    fsm->toDot("NN");
    
    Fsm fsmMin = fsm->minimiseObservableFSM();
    
    fsmMin.toDot("NN_MIN");
    
}


void test15() {
    
    cout << "TC-DFSM-0015 Show that Fsm::transformToObservableFSM() produces an "
    << "equivalent observable FSM"
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    shared_ptr<Fsm> nonObs = make_shared<Fsm>("../../../resources/nonObservable.fsm",pl,"NON_OBS");
    
    
    nonObs->toDot("NON_OBS");
    
    Fsm obs = nonObs->transformToObservableFSM();
    
    obs.toDot("OBS");
    
    assert("TC-DFSM-0015",
           obs.isObservable(),
           "Transformed FSM is observable");
    
    // Show that nonObs and obs have the same language.
    // We use brute force test that checks all traces of length n*m
    int n = (int)nonObs->size();
    int m = (int)obs.size();
    int theLen = n+m-1;
    
    IOListContainer allTrc = IOListContainer(nonObs->getMaxInput(),
                                             1,
                                             theLen,
                                             pl);
    
    shared_ptr<vector<vector<int>>> allTrcLst = allTrc.getIOLists();
    
    for ( auto trc : *allTrcLst ) {
        
        // Run the test case against both FSMs and compare
        // the (nondeterministic) result
        shared_ptr<InputTrace> iTr =
        make_shared<InputTrace>(trc,pl);
        OutputTree o1 = nonObs->apply(*iTr);
        OutputTree o2 = obs.apply(*iTr);
        
        if ( o1 != o2 ) {
            
            assert("TC-DFSM-0015",
                   o1 == o2,
                   "Transformed FSM has same language as original FSM");
            
            cout << "o1 = " << o1 << endl;
            cout << "o2 = " << o2 << endl;
            return;
            
        }
       
    }
    
}

void faux() {
    
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>("../../../resources/gillIn.txt",
                                      "../../../resources/gillOut.txt",
                                      "../../../resources/gillState.txt");
    
    shared_ptr<Dfsm> d = make_shared<Dfsm>("../../../resources/gill.fsm",
                                           pl,
                                           "G0");
    
    d->toDot("G0");
    
    d->toCsv("G0");
    
    Dfsm dMin = d->minimise();
    
    dMin.toDot("G0_MIN");
    
    
    
}

void test16() {
    
    shared_ptr<Dfsm> exp1 = nullptr;
    Reader jReader;
    Value root;
    stringstream document;
    ifstream inputFile("../../../resources/exp1.fsm");
    document << inputFile.rdbuf();
    inputFile.close();
    
    if ( jReader.parse(document.str(),root) ) {
        exp1 = make_shared<Dfsm>(root);
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    exp1->toDot("exp1");
    
    shared_ptr<Dfsm> exp2 = nullptr;
    Reader jReader2;
    Value root2;
    stringstream document2;
    ifstream inputFile2("../../../resources/exp2.fsm");
    document2 << inputFile2.rdbuf();
    inputFile2.close();
    
    if ( jReader2.parse(document2.str(),root) ) {
        exp2 = make_shared<Dfsm>(root);
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    exp2->toDot("exp2");
    
    Fsm prod = exp1->intersect(*exp2);
    
    cout << endl << "NEW PL STATES" << endl ;
    prod.getPresentationLayer()->dumpState(cout);
    
    
    prod.toDot("PRODexp1exp2");
    
    
    
    
}

bool checkDistinguishingCond(Dfsm &minimized) {
	const std::vector<shared_ptr<FsmNode>> nodes = minimized.getNodes();
	for (size_t i = 0; i < nodes.size() - 1; ++i) {
		for (size_t j = i + 1; j < nodes.size(); ++j) {
			if (!minimized.distinguishable(*nodes[i], *nodes[j])) {
				return false;
			}
		}
	}
	return true;
}


void testMinimise() {
	cout << "TC-DFSM-0017 Show that Dfsm::minimise() produces an "
		<< "equivalent minimal FSM"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto dfsm = make_shared<Dfsm>("DFSM", 50, 5, 5, pl);
	Dfsm minimized = dfsm->minimise();
	std::vector<shared_ptr<FsmNode>> unreachableNodes;

	// check for unreachable nodes
	assert("TC-DFSM-0017",
		not minimized.removeUnreachableNodes(unreachableNodes),
		"Minimized Dfsm doesn't contain unreachable nodes");

	// check if states are distinguishable
	assert("TC-DFSM-0017",
		checkDistinguishingCond(minimized),
		"Each node pair of the minimized Dfsm is distinguishable");

	// check language equality
	assert("TC-DFSM-0017",
		minimized.intersect(*dfsm).isCompletelyDefined(),
		"Language of minimized Dfsm equals language of unminimized Dfsm");
}

void testWMethod() {
	cout << "TC-DFSM-0018 Show that Dfsm implModel only passes W-Method Testsuite "
		<< "if intersection is completely defined"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto refModel = make_shared<Dfsm>("refModel", 50, 5, 5, pl);
	auto implModel = make_shared<Dfsm>("implModel", 50, 5, 5, pl);
	IOListContainer iolc = refModel->wMethod(0);

	// check language equality with W-Method Testsuite
	bool equal = true;
	for (auto trc : *(iolc.getIOLists())) {
		shared_ptr<InputTrace> iTr =
			make_shared<InputTrace>(trc, pl);
		if (not implModel->pass(refModel->applyDet(*iTr))) {
			equal = false;
			break;
		}
	}

	assert("TC-DFSM-0018",
		refModel->intersect(*implModel).isCompletelyDefined() == equal,
		"implModel passes W-Method Testsuite if and only if intersection is completely defined");
}

// Checks if tr1 is a prefix of tr2.
bool isPrefix(const std::vector<int> &tr1, const std::vector<int> &tr2) {
	if (tr1.size() > tr2.size()) {
		return false;
	}
	for (size_t i = 0; i < tr1.size(); ++i) {
		if (tr1[i] != tr2[i]) {
			return false;
		}
	}
	return true;
}

//Checks if ot1 is part of ot2. This is only the case if the InputTrace of ot1 is a prefix
//of the InputTrace of ot2 and if every OutputTrace of ot1 is a prefix of an OutputTrace
//of ot2.
bool containsOuputTree(OutputTree &ot1, OutputTree &ot2) {
	InputTrace it1 = ot1.getInputTrace();
	InputTrace it2 = ot2.getInputTrace();
	if (not isPrefix(it1.get(), it2.get())) {
		return false;
	}

	for (OutputTrace &outTr1 : ot1.getOutputTraces()) {
		bool prefix(false);
		for (OutputTrace &outTr2 : ot2.getOutputTraces()) {
			if (isPrefix(outTr1.get(), outTr2.get())) {
				prefix = true;
				break;
			}
		}
		if (not prefix) {
			return false;
		}
	}
	return true;
}

//void testIntersect() {
//	cout << "TC-DFSM-0019 Show that Fsm::intersect() produces FSM which accepts intersection "
//		<< "of the languages from the original FSMs"
//		<< endl;
//
//	auto pl = make_shared<FsmPresentationLayer>();
//	auto m1 = make_shared<Dfsm>("refModel", 3, 3, 3, pl)->minimise();
//	auto m2 = m1.createMutant("mutant", 2, 2)->minimise();
//	Fsm intersection = m1.intersect(m2).minimise();
//
//	std::cout << "m1.isDeterministic(): " << m1.isDeterministic() << std::endl;
//	std::cout << "m1.isCompletelyDefined(): " << m1.isCompletelyDefined() << std::endl;
//
//	std::cout << "m2.isDeterministic(): " << m2.isDeterministic() << std::endl;
//	std::cout << "m2.isCompletelyDefined(): " << m2.isCompletelyDefined() << std::endl;
//
//	std::cout << "intersection.isDeterministic(): " << intersection.isDeterministic() << std::endl;
//	std::cout << "intersection.isCompletelyDefined(): " << intersection.isCompletelyDefined() << std::endl;
//
//	std::cout << "m1.size(): " << m1.size();
//	std::cout << "intersection.size(): " << intersection.size();
//
//	// show that every trace of length n+m-1 in language of intersection is in language of m1 and m2 
//	IOListContainer iolc = IOListContainer(m1.getMaxInput(),
//			1,
//			intersection.size() + m1.size() - 1,
//			pl);
//
//	std::cout << "iolc created" << std::endl;
//
//	int c = 0;
//	for (auto trc : *(iolc.getIOLists())) {
//		if (c++ >= 10) break;
//		shared_ptr<InputTrace> iTr =
//			make_shared<InputTrace>(trc, pl);
//		OutputTree ot1 = intersection.apply(*iTr);
//		OutputTree ot2 = m1.apply(*iTr);
//		OutputTree ot3 = m2.apply(*iTr);		
//
//		std::cout << "ot2.contains(ot1): " << ot2.contains(ot1) << std::endl;
//		std::cout << "ot3.contains(ot1): " << ot3.contains(ot1) << std::endl;
//
//		std::cout << "containsOuputTree(ot1, ot2)" << containsOuputTree(ot1, ot2) << std::endl;
//		std::cout << "containsOuputTree(ot1, ot3)" << containsOuputTree(ot1, ot3) << std::endl;
//
//		std::cout << "intersection tr:\t" << ot1 << std::endl;
//		std::cout << std::endl;
//		std::cout << "ot2:\t\t\t" << ot2 << std::endl;
//		std::cout << std::endl;
//		std::cout << "ot3:\t\t\t" << ot3 << std::endl;
//	}
//}

void testIntersectionCharacteristics() {
	auto pl = make_shared<FsmPresentationLayer>();
	auto m1 = make_shared<Dfsm>("m1", 10, 3, 3, pl)->minimise();
	auto m2 = m1.createMutant("m2", 2, 2);

	assert("TC-DFSM-0019b",
		m1.intersect(*m2).isDeterministic(),
		"m1 or m2 deterministic => product automata deterministic");

	auto m3 = Fsm::createRandomFsm("m3", 3, 3, 3, pl);
	auto m4 = Fsm::createRandomFsm("m4", 3, 3, 3, pl);
	Fsm intersection = m3->intersect(*m4);
	if (not intersection.isDeterministic()) {
		assert("TC-DFSM-0019b",
			(not m3->isDeterministic()) and (not m4->isDeterministic()),
			"product automata of m3 and m4 nondeterministic => m3 and m4 nondeterministic");
	}
	if (intersection.isCompletelyDefined()) {
		assert("TC-DFSM-0019b",
			m3->isCompletelyDefined() and m4->isCompletelyDefined(),
			"product automata of m3 and m4 completely specified => m3 and m4 completely specified");
	}

	// TODO: m1 or m2 incomplete specified => product automata incomplete specified
}

bool equalSetOfOutputTrees(std::vector<OutputTree> &otv1, std::vector<OutputTree> &otv2) {
	if (otv1.size() != otv2.size()) {
		return false;
	}
	for (size_t i = 0; i < otv1.size(); ++i) {
		if (otv1[i] != otv2[i]) {
			return false;
		}
	}
	return true;
}

void testCharacterisationSet() {
	cout << "TC-FSM-0019 Show that calculated characterisation set "
		<< "distinguishes each pair of FSM states"
		<< endl;
	auto pl = make_shared<FsmPresentationLayer>();
	auto m1 = Fsm::createRandomFsm("M1", 3, 3, 10, pl)->minimise();
	IOListContainer iolc = m1.getCharacterisationSet();

	// calculate output trees for every node
	std::vector<std::vector<OutputTree>> outputTrees;
	for (const auto &node : m1.getNodes()) {		
		std::vector<OutputTree> traces;
		for (const auto trc : *(iolc.getIOLists())) {
			shared_ptr<InputTrace> iTr =
				make_shared<InputTrace>(trc, pl);
			traces.push_back(node->apply(*iTr));
		}
		outputTrees.push_back(traces);
	}

	// check if vector contains equal sets of output trees
	for (size_t i = 0; i < m1.getNodes().size() - 1; ++i) {
		for (size_t j = i + 1; j < m1.getNodes().size(); ++j) {
			if (equalSetOfOutputTrees(outputTrees[i], outputTrees[j])) {
				std::cout << "============= FAIL ==============" << std::endl;
			}
		}
	}
	std::cout << "============= PASS ============" << std::endl;
}

bool checkDistTracesForEachNodePair(Dfsm &m) {
	m.calculateDistMatrix();
	for (size_t i = 0; i < m.size() - 1; ++i) {
		for (size_t j = i + 1; j < m.size(); ++j) {
			auto ni = m.getNodes().at(i);
			auto nj = m.getNodes().at(j);
			auto distTraces = m.getDistTraces(*ni, *nj);
			for (auto trc : distTraces) {
				shared_ptr<InputTrace> iTr =
					make_shared<InputTrace>(*trc, m.getPresentationLayer());
				OutputTree oti = ni->apply(*iTr);
				OutputTree otj = nj->apply(*iTr);
				if (oti == otj) {
					return false;
				}
			}
		}
	}
	return true;
}

void testGetDistTraces() {
	cout << "TC-DFSM-0020 Show that calculated distinguishing traces "
		<< "in fact distinguish states"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto m = make_shared<Dfsm>("M", 50, 5, 5, pl);

	assert("TC-DFSM-0020",
		checkDistTracesForEachNodePair(*m),
		"Each calculated distinguishing trace produces unequal set of output traces");

}

void testHMethod() {
	cout << "TC-DFSM-0021 Show that Dfsm implModel only passes H-Method Testsuite "
		<< "if intersection is completely defined"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto refModel = make_shared<Dfsm>("refModel", 50, 5, 5, pl)->minimise();
	Fsm implModel = refModel.createMutant("mutant", 2, 2)->minimise();

	// refModel and implModel have to be compl. specified, deterministic and minimal
	// implModel should have at most the same size as refModel
	IOListContainer iolc = refModel.hMethodOnMinimisedDfsm(0);
	TestSuite ts1 = refModel.createTestSuite(iolc);
	TestSuite ts2 = implModel.createTestSuite(iolc);

	assert("TC-DFSM-0021",
		refModel.intersect(implModel).isCompletelyDefined() == ts1.isEquivalentTo(ts2),
		"implModel passes H-Method Testsuite if and only if intersection is completely defined");
}

void testWpMethodWithDfsm() {
	cout << "TC-DFSM-0022 Show that Dfsm implModel only passes Wp-Method Testsuite "
		<< "if intersection is completely defined"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto refModel = make_shared<Dfsm>("refModel", 50, 5, 5, pl)->minimise();
	Fsm implModel = refModel.createMutant("mutant", 1, 1)->minimise();

	// refModel required to be minimised and observable
	// implModel required to have at most 0 additional states compared to refModel (both are prime machines)
	IOListContainer iolc = refModel.wpMethodOnMinimisedDfsm(0);
	TestSuite ts1 = refModel.createTestSuite(iolc);
	TestSuite ts2 = implModel.createTestSuite(iolc);

	// refModel and implModel required to be deterministic and completely specified
	assert("TC-DFSM-0022",
		refModel.intersect(implModel).isCompletelyDefined() == ts1.isEquivalentTo(ts2),
		"implModel passes Wp-Method Testsuite if and only if intersection is completely defined");
}


//===================================== TreeNode Tests ===================================================

// tests TreeNode::add(const int x). Checks if new TreeEdge is created for given input.
void testTreeNodeAddConstInt1(){
	int io = 1;
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> ref = n1->add(io);
	assert("TC-TreeNode-NNNN",
		static_cast<shared_ptr<TreeNode>>(ref->getParent()) == n1,
		"parent of new node is old node");

	bool containedInChildren = false;
	for (shared_ptr<TreeEdge> e : *(n1->getChildren())) {
		if (e->getIO() == io && e->getTarget() == ref) {
			containedInChildren = true;
		}
	}
	assert("TC-TreeNode-NNNN",
		containedInChildren,
		"after call to TreeNode::add(x) there has to be a child labeled with x");
}

// tests TreeNode::add(const int x). Checks if no new TreeEdge is created if TreeEdge with matching io label
// already exists.
void testTreeNodeAddConstInt2() {
	int io = 1;
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = n1->add(io);
	int oldNumChilds = n1->getChildren()->size();
	shared_ptr<TreeNode> child2 = n1->add(io);
	int newNumChilds = n1->getChildren()->size();
	assert("TC-TreeNode-NNNN",
		child2 == child1,
		"TreeNode::add(x) returns reference to target node of existing TreeEdge with matching io");
	assert("TC-TreeNode-NNNN",
		oldNumChilds == newNumChilds,
		"TreeNode::add(x) doesn't add new TreeEdge if TreeEdge with matching io already exists");
}

// tests TreeNode::add(const int x). Checks if new TreeEdge is created for given input. TreeNode already has children, but none with matching
// io label.
void testTreeNodeAddConstInt3() {
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = n1->add(1);
	shared_ptr<TreeNode> child2 = n1->add(2);
	assert("TC-TreeNode-NNNN",
		child1 != child2,
		"calling TreeNode::add(x) and TreeNode::add(y) with x != y returns two different nodes");


	assert("TC-TreeNode-NNNN",
		n1->getChildren()->size() == 2,
		"number of TreeEdges contained in children attribute matches number of actually added values");
}

// tests operator==(TreeNode const & treeNode1, TreeNode const & treeNode2)  (positive case)
void testTreeNodeEqualOperator1() {
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n2 = make_shared<TreeNode>();
	assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");

	shared_ptr<TreeNode> n11 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n21 = make_shared<TreeNode>();
	n1->add(make_shared<TreeEdge>(1, n11));
	n2->add(make_shared<TreeEdge>(1, n21));
	assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");

	shared_ptr<TreeNode> n12 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n22 = make_shared<TreeNode>();
	n1->add(make_shared<TreeEdge>(2, n12));
	n2->add(make_shared<TreeEdge>(2, n22));
	assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");

	shared_ptr<TreeNode> n111 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n112 = make_shared<TreeNode>();
	n11->add(make_shared<TreeEdge>(1, n111));
	n11->add(make_shared<TreeEdge>(2, n112));
	shared_ptr<TreeNode> n211 = make_shared<TreeNode>();	
	shared_ptr<TreeNode> n212 = make_shared<TreeNode>();
	n21->add(make_shared<TreeEdge>(1, n211));
	n21->add(make_shared<TreeEdge>(2, n212));

	assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");
}

// tests operator==(TreeNode const & treeNode1, TreeNode const & treeNode2)  (negative case)
void testTreeNodeEqualOperator2() {
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n2 = make_shared<TreeNode>();
	n1->deleteSingleNode();
	assert("TC-TreeNode-NNNN",
		!(*n1 == *n2),
		"operator== returns false if only one of the TreeNode instances is marked as deleted");

	n1 = make_shared<TreeNode>();
	n2 = make_shared<TreeNode>();
	n2->add(make_shared<TreeEdge>(1, make_shared<TreeNode>()));
	assert("TC-TreeNode-NNNN",
		!(*n1 == *n2),
		"operator== returns false if the compared TreeNode instances have different number of children");

	n1->add(make_shared<TreeEdge>(2, make_shared<TreeNode>()));
	assert("TC-TreeNode-NNNN",
		!(*n1 == *n2) && n1->getChildren()->size() == n2->getChildren()->size(),
		"operator== returns false if both TreeNode instances have same number of children but edges are labeled differently");

	n1 = make_shared<TreeNode>();
	n2 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n11 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n21 = make_shared<TreeNode>();
	n1->add(make_shared<TreeEdge>(1, n11));
	n2->add(make_shared<TreeEdge>(1, n21));
	n11->add(make_shared<TreeEdge>(1, make_shared<TreeNode>()));
	assert("TC-TreeNode-NNNN",
		!(*n1 == *n2) && n11->getChildren()->size() != n21->getChildren()->size(),
		"operator== returns false if two corresponding childs of both TreeNode instances differ in the number of children");

	n21->add(make_shared<TreeEdge>(2, make_shared<TreeNode>()));
	assert("TC-TreeNode-NNNN",
		!(*n1 == *n2) && n11->getChildren()->size() == n21->getChildren()->size(),
		"operator== returns false if two corresponding childs of both TreeNode instances differ in the labeling of their children");

	n11->add(make_shared<TreeEdge>(2, make_shared<TreeNode>()));
	n21->add(make_shared<TreeEdge>(1, make_shared<TreeNode>()));
	n11->deleteSingleNode();
	assert("TC-TreeNode-NNNN",
		!(*n1 == *n2),
		"operator== returns false if two corresponding childs differ in beeing marked as deleted");
}


int main(int argc, char** argv)
{
    
    
    
#if 0
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    test10b();
    test11();
    test13();
    test14();
    test15();

    faux();

    
    gdc_test1();
    
    

    wVersusT();

    if ( argc < 6 ) {
        cerr << endl <<
        "Missing file names - exit." << endl;
        exit(1);
    }
    
    
    
    string fsmName(argv[1]);
    string fsmFile(argv[2]);
    
    /*
    string inputFile(argv[3]);
    string outputFile(argv[4]);
    string stateFile(argv[5]);
    */
    
    /* Create the presentation layer */
    //shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>(inputFile,outputFile,stateFile);
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    /* Create an Fsm instance, using the transition relation file,
     * the presentation layer, and the FSM name
     */
    shared_ptr<Fsm> fsm = make_shared<Fsm>(fsmFile,pl,fsmName);
    
    /* Produce a GraphViz (.dot) representation of the created FSM */
    fsm->toDot(fsmName);
    
    /* Transform the FSM into an equivalent observable one */
    Fsm fsmObs = fsm->transformToObservableFSM();
    
    /* Output the observable FSM to a GraphViz file (.dot-file) */
    fsmObs.toDot(fsmObs.getName());
    
#endif
	//testTreeNodeAddConstInt3();
	testTreeNodeEqualOperator2();

	/*testMinimise();
	testWMethod();*/
	//testCharacterisationSet();
	//testGetDistTraces();
	//testHMethod();
	//testWpMethodWithDfsm(); 
	//testIntersectionCharacteristics();
    //test1();
    //test2();
    //test3();
    //test4();
    //test5();
    //test6();
    //test7();
    //test8();
    //test9();
    //test10();
    //test10b();
    //test11();
    //test13();
    //test14();
    //test15();
    exit(0);
    
}



