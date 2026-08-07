// tbldic.cpp -- the PRINT and SAVE table dictionaries (table.prm/table.var and
// stable.prm/stable.var) and the four-way lookup getprt.f/getsav.f do over
// them, factored out of readers_val.cpp because the eight tables are 11k
// characters of generated data.
//
// Every X-13 table has a LONG and a SHORT name written adjacent, so entries
// 2i-1 and 2i map to table i; each spec owns a slice given by tbllog.i's
// LSP<spec>/NSP<spec> pair (in specparse.hpp). The dictionary is split in FOUR
// at BRKDSP/BRKDS2/BRKDS3 for a reason that no longer exists -- table.prm says
// it keeps each literal "under 2000 characters, a requirement for the VAX/VMS
// Fortran" -- and the split is reproduced because the displacements are
// relative to each piece.
//
// PRINT and SAVE are DIFFERENT dictionaries over the same 396 table slots:
// stable.prm's entries are empty strings wherever a table can be printed but
// not saved (check{}'s `acfplot`/`acp`, history{}'s `header`/`hdr`). An empty
// entry can never match, because gtdcnm only looks anything up for a NAME
// token -- so the emptiness IS the refusal, and it is transcribed as such.
#include "specparse/specparse.hpp"

namespace x13 {

using namespace tbllog;

namespace {

static const char PRT_TB1DIC[] =
    "headerhdrspana1seriesplota1pspecfilespcsavefilesavseriesmvadjmvcalen"
    "daradjoriga18outlieradjoriga19adjoriginalb1adjorigplotb1pseriesconst"
    "anta1cseriesconstantplotacppriora2permpriora2ptemppriora2tprioradjus"
    "teda3permprioradjusteda3pprioradjustedptda4dpermprioradjustedptda4pt"
    "ransformedtrnaictransformtacregressionmatrixrmxaictestatsoutlierotla"
    "outlieraolevelshiftlstemporarychangetcseasonaloutliersotradingdaytdh"
    "olidayholuserdefusrregseasonala10transitorya13chi2testctsdailyweight"
    "stdwacfiacacfplotacppacfipcpacfplotpcpregcoefficientsrgcheaderhdruni"
    "troottesturtautochoiceachunitroottestmdlurmautochoicemdlamdbestfivem"
    "dlb5mautooutlierhdraohautooutlieritraoiautooutliertestsaotautofinalo"
    "utliertestsaftautodefaulttestsadtautoljungboxtestalbautofinaltestsaf"
    "theaderhdrheaderbcsthdbusermodelsumdpickmdlchoicepchoptionsoptiterat"
    "ionsitriterationerrorsitemodelmdlregcmatrixrcmestimatesestarmacmatri"
    "xacmlkstatslkslformulaslkfrootsrtsregressioneffectsrefresidualsrsdre"
    "gressionresidualsrrsaveragefcsterrafcheaderhdriterationsoittestsotst"
    "emporarylstlsfinaltestsftsacfacfacfplotacppacfpcfpacfplotpcpacfsquar"
    "edac2acfsquaredplotap2histogramhstnormalitytestnrmdurbinwatsondwfrie"
    "dmantestfrtinvpacfinptransformedftrvariancesfvrforecastsfcttransform"
    "edbcstbtrbackcastsbctspecorigsp0specresidualsprspecsasp1specirrsp2sp"
    "ecseatssas1sspecseatsirrs2sspecextresidualsserspecindsais1specindirr"
    "is2speccompositeis0spectukeyorigst0spectukeyresidualstrspectukeysast"
    "1spectukeyirrst2spectukeyseatssat1sspectukeyseatsirrt2sspectukeyextr"
    "esidualsterspectukeyindsait1spectukeyindirrit2spectukeycompositeit0q"
    "sqsqsindqsitukeypeakstpkqcheckqchnpsanpanpsaindnpi";

static const int PRT_tb1ptr[237] = {
    1, 7, 10, 14, 16, 26, 29, 37, 40, 48, 51, 62, 64, 79, 82, 96, 99, 110,
    112, 123, 126, 140, 143, 161, 164, 169, 171, 180, 183, 192, 195, 208,
    210, 227, 230, 246, 249, 269, 272, 283, 286, 298, 301, 317, 320, 327,
    330, 337, 340, 348, 350, 360, 362, 377, 379, 394, 396, 406, 408, 415,
    418, 425, 428, 439, 442, 452, 455, 463, 466, 478, 481, 484, 487, 494,
    497, 501, 504, 512, 515, 530, 533, 539, 542, 554, 557, 567, 570, 585,
    588, 601, 604, 615, 618, 632, 635, 649, 652, 668, 671, 692, 695, 711,
    714, 730, 733, 747, 750, 756, 759, 769, 772, 782, 785, 798, 801, 808,
    811, 821, 824, 839, 842, 847, 850, 860, 863, 872, 875, 886, 889, 896,
    899, 908, 911, 916, 919, 936, 939, 948, 951, 970, 973, 987, 990, 996,
    999, 1009, 1012, 1017, 1020, 1031, 1034, 1044, 1047, 1050, 1053, 1060,
    1063, 1067, 1070, 1078, 1081, 1091, 1094, 1108, 1111, 1120, 1123, 1136,
    1139, 1151, 1153, 1165, 1168, 1175, 1178, 1189, 1192, 1201, 1204, 1213,
    1216, 1231, 1234, 1243, 1246, 1254, 1257, 1269, 1272, 1278, 1281, 1288,
    1291, 1302, 1305, 1317, 1320, 1336, 1339, 1348, 1351, 1361, 1364, 1377,
    1380, 1393, 1396, 1413, 1416, 1427, 1430, 1442, 1445, 1461, 1464, 1481,
    1484, 1505, 1508, 1522, 1525, 1540, 1543, 1561, 1564, 1566, 1568, 1573,
    1576, 1586, 1589, 1595, 1598, 1602, 1605, 1612, 1615
};

static const char PRT_TB2DIC[] =
    "adjoriginalcc1adjoriginaldd1modoriginale1mcdmovavgf1trendb2b2trendc2"
    "c2trendd2d2modseasadje2sib3b3modirregulare3replacsib4b4modsic4c4mods"
    "id4d4seasonalb5b5seasonalc5c5seasonald5d5origchangese5origchangespct"
    "pe5seasadjb6b6seasadjc6c6seasadjd6d6sachangese6sachangespctpe6trendb"
    "7b7trendc7c7trendd7d7trendchangese7trendchangespctpe7sib8b8unmodsid8"
    "unmodsioxd8bcalendaradjchangese8calendaradjchangespctpe8replacsib9b9"
    "replacsic9c9replacsid9seasonalb10b10seasonalc10c10seasonald10seasona"
    "lpctpsfseasonaldifffsdseasonaladjregseaarsseasonalnoshrinksnsseasadj"
    "b11b11seasadjc11c11seasadjd11seasadjconstsacrobustsae11trendd12trend"
    "adjlstalbiasfactorbcftrendconsttacirregularbb13irregularcc13irregula"
    "rd13irregularpctpirirregularadjaoirairrwtbb17irrwtc17extremebb20extr"
    "emec20x11easterh1combholidaychladjustfacd16adjustfacpctpafadjustdiff"
    "fadcalendard18adjustmentratioe18totaladjustmenttadtdadjorigbb19tdadj"
    "origc19ftestb1b1fx11diagf2qstatf3yrtotalse4ftestd8d8fmovseasratd9are"
    "sidualseasfrsfautosfasftdaytypetdyorigwsaplote0ratioplotorigra1ratio"
    "plotsara2seasonalplotsfpseasadjplotsaptrendplottrpirregularplotirpse"
    "asadjfcstsaftrendfcsttrfirrwtfcstiwfseasadjtotsaasaroundrndrevsachan"
    "gese6arevsachangespctp6arndsachangese6rrndsachangespctp6rcratiocrrra"
    "tiorrforcefactorffcpriortda4extremevalbb14extremevalc14x11regbb15x11"
    "regc15tradingdaybb16tradingdayc16combtradingdaybb18combtradingdayc18"
    "holidaybbxhholidayxhlcalendarbbxccalendarxcacombcalendarbbcccombcale"
    "ndarxccoutlierhdrxohoutlieriterxoioutliertestsxotoutlierfinaltestsxf"
    "txregressionmatrixxrmxregressioncmatrixxrcxaictestxatheaderhdroutlie"
    "rhistoryrotsfilterhistorysfhsarevisionssarsasummarysassaestimatessae"
    "chngrevisionschrchngsummarychschngestimatescheindsarevisionsiarindsa"
    "summaryiasindsaestimatesiaetrendrevisionstrrtrendsummarytrstrendesti"
    "matestretrendchngrevisionstcrtrendchngsummarytcstrendchngestimatestc"
    "esfrevisionssfrsfsummarysfssfestimatessfelkhdhistorylkhfcsterrorsfce"
    "fcsthistoryfchseatsmdlhistorysmhseasonalfcthistoryssharmahistoryamht"
    "dhistorytdh";

static const int PRT_tb2ptr[299] = {
    1, 13, 15, 27, 29, 40, 42, 51, 53, 60, 62, 69, 71, 78, 80, 90, 92, 96,
    98, 110, 112, 122, 124, 131, 133, 140, 142, 152, 154, 164, 166, 176,
    178, 189, 191, 205, 208, 217, 219, 228, 230, 239, 241, 250, 252, 264,
    267, 274, 276, 283, 285, 292, 294, 306, 308, 323, 326, 330, 332, 339,
    341, 350, 353, 371, 373, 394, 397, 407, 409, 419, 421, 429, 431, 442,
    445, 456, 459, 467, 470, 481, 484, 496, 499, 516, 519, 535, 538, 548,
    551, 561, 564, 571, 574, 586, 589, 597, 600, 605, 608, 618, 621, 631,
    634, 644, 647, 657, 660, 670, 673, 682, 685, 697, 700, 714, 717, 723,
    726, 731, 734, 742, 745, 752, 755, 764, 766, 777, 780, 789, 792, 804,
    807, 817, 820, 828, 831, 846, 849, 864, 867, 877, 880, 889, 892, 899,
    902, 909, 911, 916, 918, 926, 928, 935, 938, 948, 951, 964, 967, 973,
    976, 984, 987, 998, 1000, 1013, 1016, 1027, 1030, 1042, 1045, 1056,
    1059, 1068, 1071, 1084, 1087, 1098, 1101, 1110, 1113, 1122, 1125, 1135,
    1138, 1145, 1148, 1160, 1163, 1178, 1181, 1193, 1196, 1211, 1214, 1220,
    1222, 1228, 1230, 1241, 1244, 1251, 1253, 1264, 1267, 1277, 1280, 1287,
    1290, 1296, 1299, 1310, 1313, 1323, 1326, 1341, 1344, 1358, 1361, 1369,
    1372, 1379, 1382, 1391, 1394, 1402, 1405, 1418, 1421, 1433, 1436, 1446,
    1449, 1460, 1463, 1475, 1478, 1495, 1498, 1515, 1518, 1536, 1539, 1547,
    1550, 1556, 1559, 1573, 1576, 1590, 1593, 1604, 1607, 1616, 1619, 1630,
    1633, 1646, 1649, 1660, 1663, 1676, 1679, 1693, 1696, 1708, 1711, 1725,
    1728, 1742, 1745, 1757, 1760, 1774, 1777, 1795, 1798, 1814, 1817, 1835,
    1838, 1849, 1852, 1861, 1864, 1875, 1878, 1889, 1892, 1902, 1905, 1916,
    1919, 1934, 1937, 1955, 1958, 1969, 1972, 1981, 1984
};

static const char PRT_TB3DIC[] =
    "headerhdrssftestssffactormeansfmnindfactormeansfmipercentpctindperce"
    "ntpciyypercentpcyindyypercentpiysummarysumindsummarysmiyysummarysuyi"
    "ndyysummarysiysfspanssfsindsfspanssischngspanschsindchngspanscissasp"
    "ansadsindsaspansaisychngspansycsindychngspansyistdspanstdscomposites"
    "rscmsprioradjcompositeia3adjcompositesrsb1adjcompositeplotb1pcalenda"
    "radjcompositecacoutlieradjcompositeoacheaderhdrindtestittindunmodsii"
    "d8indreplacsiid9indseasonalisfindseasonalpctipsindseasonaldiffisdind"
    "seasadjisaindtrenditnindirregulariirindirregularpctipiindmodoriginal"
    "ie1indmodsadjie2indmodirrie3origchangesie5origchangespctip5indsachan"
    "gesie6indsachangespctip6indrevsachangesi6aindrevsachangespctipaindrn"
    "dsachangesi6rindrndsachangespctiprindtrendchangesie7indtrendchangesp"
    "ctip7indcalendaradjchangesie8indcalendaradjchangespctip8indrobustsai"
    "eeindadjustmentratioi18indtotaladjustmentitaindmcdmovavgif1indx11dia"
    "gif2indqstatif3indyrtotalsie4indftestd8idfindmovseasratimsindresidua"
    "lseasfirfindadjsatotiaaindsadjroundirncompositeplotcmporigwindsaplot"
    "ie0ratioplotorigir1ratioplotindsair2indseasonalplotispindseasadjplot"
    "iapindtrendplotitpindirregularplotiipindlevelshiftilsindaoutlieriaoi"
    "ndcalendaricaindadjustfaciafindadjustfacpctipfindcratiocriindrratior"
    "riindforcefactoriff";

static const int PRT_tb3ptr[163] = {
    1, 7, 10, 17, 20, 31, 34, 48, 51, 58, 61, 71, 74, 83, 86, 98, 101, 108,
    111, 121, 124, 133, 136, 148, 151, 158, 161, 171, 174, 183, 186, 198,
    201, 208, 211, 221, 224, 234, 237, 250, 253, 260, 263, 275, 278, 295,
    298, 313, 315, 331, 334, 354, 357, 376, 379, 385, 388, 395, 398, 408,
    411, 422, 425, 436, 439, 453, 456, 471, 474, 484, 487, 495, 498, 510,
    513, 528, 531, 545, 548, 558, 561, 570, 573, 584, 587, 601, 604, 616,
    619, 634, 637, 652, 655, 673, 676, 691, 694, 712, 715, 730, 733, 751,
    754, 775, 778, 802, 805, 816, 819, 837, 840, 858, 861, 873, 876, 886,
    889, 897, 900, 911, 914, 924, 927, 940, 943, 959, 962, 973, 976, 988,
    991, 1004, 1007, 1021, 1024, 1037, 1040, 1054, 1057, 1072, 1075, 1089,
    1092, 1104, 1107, 1123, 1126, 1139, 1142, 1153, 1156, 1167, 1170, 1182,
    1185, 1200, 1203, 1212, 1215, 1224, 1227, 1241, 1244
};

static const char PRT_TB4DIC[] =
    "trends12trendconststcseasonals10seasonalpctpssirregulars13irregularp"
    "ctpsiseasonaladjs11seasadjconstsectransitorys14transitorypctpscadjus"
    "tfacs16adjustfacpctpsatrendfcstdecomptfdseasonalfcstdecompsfdseriesf"
    "cstdecompofdseasonaladjfcstdecompafdtransitoryfcstdecompyfdadjustmen"
    "tratios18totaladjustmentstawkendfilterwkfcomponentmodelsmdcpseudoinn"
    "ovtrendpicpseudoinnovseasonalpispsuedoinnovtransitorypitpsuedoinnovs"
    "adjpiasquaredgainsasymgafsquaredgainsaconcgacsquaredgaintrendsymgtfs"
    "quaredgaintrendconcgtctimeshiftsaconctactimeshifttrendconcttcfilters"
    "asymfaffiltersaconcfacfiltertrendsymftffiltertrendconcftcdifforigina"
    "ldordiffseasonaladjdsadifftrenddtrseasonalsumssmcyclecyclongtermtren"
    "dlttseasonalsesseseasonaladjseasetrendsetsetransitorysecseseasonalad"
    "joutlieradjse2irregularoutlieradjse3trendadjlsstl";

static const int PRT_tb4ptr[97] = {
    1, 6, 9, 19, 22, 30, 33, 44, 47, 56, 59, 71, 74, 85, 88, 100, 103, 113,
    116, 129, 132, 141, 144, 156, 159, 174, 177, 195, 198, 214, 217, 238,
    241, 261, 264, 279, 282, 297, 300, 311, 314, 329, 332, 348, 351, 370,
    373, 394, 397, 412, 415, 431, 434, 451, 454, 473, 476, 496, 499, 514,
    517, 535, 538, 549, 552, 564, 567, 581, 584, 599, 602, 614, 617, 632,
    635, 644, 647, 658, 661, 666, 669, 682, 685, 695, 698, 711, 714, 721,
    724, 736, 739, 760, 763, 782, 785, 795, 798
};

static const char SAV_TB1DIC[] =
    "spana1specfilespcseriesmvadjmvcalendaradjoriga18outlieradjoriga19adj"
    "originalb1seriesconstanta1cpriora2permpriora2ptemppriora2tprioradjus"
    "teda3permprioradjusteda3pprioradjustedptda4dpermprioradjustedptda4pt"
    "ransformedtrnregressionmatrixrmxoutlierotlaoutlieraolevelshiftlstemp"
    "orarychangetcseasonaloutliersotradingdaytdholidayholuserdefusrregsea"
    "sonala10transitorya13acfiacpacfipciterationsitrmodelmdlregcmatrixrcm"
    "estimatesestarmacmatrixacmlkstatslksrootsrtsregressioneffectsrefresi"
    "dualsrsdregressionresidualsrrsiterationsoitfinaltestsftsacfacfpacfpc"
    "facfsquaredac2transformedftrvariancesfvrforecastsfcttransformedbcstb"
    "trbackcastsbctspecorigsp0specresidualsprspecsasp1specirrsp2specseats"
    "sas1sspecseatsirrs2sspecextresidualsserspecindsais1specindirris2spec"
    "compositeis0spectukeyorigst0spectukeyresidualstrspectukeysast1spectu"
    "keyirrst2spectukeyseatssat1sspectukeyseatsirrt2sspectukeyextresidual"
    "sterspectukeyindsait1spectukeyindirrit2spectukeycompositeit0";

static const int SAV_tb1ptr[237] = {
    1, 1, 1, 5, 7, 7, 7, 15, 18, 18, 18, 29, 31, 46, 49, 63, 66, 77, 79, 79,
    79, 93, 96, 96, 96, 101, 103, 112, 115, 124, 127, 140, 142, 159, 162,
    178, 181, 201, 204, 215, 218, 218, 218, 234, 237, 237, 237, 244, 247,
    255, 257, 267, 269, 284, 286, 301, 303, 313, 315, 322, 325, 332, 335,
    346, 349, 359, 362, 362, 362, 362, 362, 365, 368, 368, 368, 372, 375,
    375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375,
    375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375,
    375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 375, 385, 388,
    388, 388, 393, 396, 406, 409, 418, 421, 432, 435, 442, 445, 445, 445,
    450, 453, 470, 473, 482, 485, 504, 507, 507, 507, 507, 507, 517, 520,
    520, 520, 520, 520, 530, 533, 536, 539, 539, 539, 543, 546, 546, 546,
    556, 559, 559, 559, 559, 559, 559, 559, 559, 559, 559, 559, 559, 559,
    570, 573, 582, 585, 594, 597, 612, 615, 624, 627, 635, 638, 650, 653,
    659, 662, 669, 672, 683, 686, 698, 701, 717, 720, 729, 732, 742, 745,
    758, 761, 774, 777, 794, 797, 808, 811, 823, 826, 842, 845, 862, 865,
    886, 889, 903, 906, 921, 924, 942, 945, 945, 945, 945, 945, 945, 945,
    945, 945, 945, 945, 945, 945
};

static const char SAV_TB2DIC[] =
    "adjoriginalcc1adjoriginaldd1modoriginale1mcdmovavgf1trendb2b2trendc2"
    "c2trendd2d2modseasadje2sib3b3modirregulare3modsic4c4modsid4d4seasona"
    "lb5b5seasonalc5c5seasonald5d5origchangese5origchangespctpe5seasadjb6"
    "b6seasadjc6c6seasadjd6d6sachangese6sachangespctpe6trendb7b7trendc7c7"
    "trendd7d7trendchangese7trendchangespctpe7sib8b8unmodsid8unmodsioxd8b"
    "calendaradjchangese8calendaradjchangespctpe8replacsic9c9replacsid9se"
    "asonalb10b10seasonalc10c10seasonald10seasonalpctpsfseasonaldifffsdse"
    "asonaladjregseaarsseasonalnoshrinksnsseasadjb11b11seasadjc11c11seasa"
    "djd11seasadjconstsacrobustsae11trendd12trendadjlstalbiasfactorbcftre"
    "ndconsttacirregularbb13irregularcc13irregulard13irregularpctpirirreg"
    "ularadjaoirairrwtbb17irrwtc17extremebb20extremec20x11easterh1combhol"
    "idaychladjustfacd16adjustfacpctpafadjustdifffadcalendard18adjustment"
    "ratioe18totaladjustmenttadtdadjorigbb19tdadjorigc19yrtotalse4seasadj"
    "fcstsaftrendfcsttrfirrwtfcstiwfseasadjtotsaasaroundrndrevsachangese6"
    "arevsachangespctp6arndsachangese6rrndsachangespctp6rcratiocrrratiorr"
    "forcefactorffcpriortda4extremevalbb14extremevalc14x11regbb15x11regc1"
    "5tradingdaybb16tradingdayc16combtradingdaybb18combtradingdayc18holid"
    "aybbxhholidayxhlcalendarbbxccalendarxcacombcalendarbbcccombcalendarx"
    "ccoutlieriterxoixregressionmatrixxrmxregressioncmatrixxrcoutlierhist"
    "oryrotsfilterhistorysfhsarevisionssarsaestimatessaechngrevisionschrc"
    "hngestimatescheindsarevisionsiarindsaestimatesiaetrendrevisionstrrtr"
    "endestimatestretrendchngrevisionstcrtrendchngestimatestcesfrevisions"
    "sfrsfestimatessfelkhdhistorylkhfcsterrorsfcefcsthistoryfchseatsmdlhi"
    "storysmhseasonalfcthistoryssharmahistoryamhtdhistorytdh";

static const int SAV_tb2ptr[299] = {
    1, 13, 15, 27, 29, 40, 42, 51, 53, 60, 62, 69, 71, 78, 80, 90, 92, 96,
    98, 110, 112, 112, 112, 119, 121, 128, 130, 140, 142, 152, 154, 164,
    166, 177, 179, 193, 196, 205, 207, 216, 218, 227, 229, 238, 240, 252,
    255, 262, 264, 271, 273, 280, 282, 294, 296, 311, 314, 318, 320, 327,
    329, 338, 341, 359, 361, 382, 385, 385, 385, 395, 397, 405, 407, 418,
    421, 432, 435, 443, 446, 457, 460, 472, 475, 492, 495, 511, 514, 524,
    527, 537, 540, 547, 550, 562, 565, 573, 576, 581, 584, 594, 597, 607,
    610, 620, 623, 633, 636, 646, 649, 658, 661, 673, 676, 690, 693, 699,
    702, 707, 710, 718, 721, 728, 731, 740, 742, 753, 756, 765, 768, 780,
    783, 793, 796, 804, 807, 822, 825, 840, 843, 853, 856, 865, 868, 868,
    868, 868, 868, 868, 868, 876, 878, 878, 878, 878, 878, 878, 878, 878,
    878, 878, 878, 878, 878, 878, 878, 878, 878, 878, 878, 878, 878, 878,
    878, 878, 878, 889, 892, 901, 904, 913, 916, 926, 929, 936, 939, 951,
    954, 969, 972, 984, 987, 1002, 1005, 1011, 1013, 1019, 1021, 1032, 1035,
    1042, 1044, 1055, 1058, 1068, 1071, 1078, 1081, 1087, 1090, 1101, 1104,
    1114, 1117, 1132, 1135, 1149, 1152, 1160, 1163, 1170, 1173, 1182, 1185,
    1193, 1196, 1209, 1212, 1224, 1227, 1227, 1227, 1238, 1241, 1241, 1241,
    1241, 1241, 1258, 1261, 1279, 1282, 1282, 1282, 1282, 1282, 1296, 1299,
    1313, 1316, 1327, 1330, 1330, 1330, 1341, 1344, 1357, 1360, 1360, 1360,
    1373, 1376, 1390, 1393, 1393, 1393, 1407, 1410, 1424, 1427, 1427, 1427,
    1441, 1444, 1462, 1465, 1465, 1465, 1483, 1486, 1497, 1500, 1500, 1500,
    1511, 1514, 1525, 1528, 1538, 1541, 1552, 1555, 1570, 1573, 1591, 1594,
    1605, 1608, 1617, 1620
};

static const char SAV_TB3DIC[] =
    "sfspanssfsindsfspanssischngspanschsindchngspanscissaspansadsindsaspa"
    "nsaisychngspansycsindychngspansyistdspanstdscompositesrscmsprioradjc"
    "ompositeia3adjcompositesrsb1calendaradjcompositecacoutlieradjcomposi"
    "teoacindunmodsiid8indreplacsiid9indseasonalisfindseasonalpctipsindse"
    "asonaldiffisdindseasadjisaindtrenditnindirregulariirindirregularpcti"
    "piindmodoriginalie1indmodsadjie2indmodirrie3origchangesie5origchange"
    "spctip5indsachangesie6indsachangespctip6indrevsachangesi6aindrevsach"
    "angespctipaindrndsachangesi6rindrndsachangespctiprindtrendchangesie7"
    "indtrendchangespctip7indcalendaradjchangesie8indcalendaradjchangespc"
    "tip8indrobustsaieeindadjustmentratioi18indtotaladjustmentitaindmcdmo"
    "vavgif1indyrtotalsie4indadjsatotiaaindsadjroundirnindlevelshiftilsin"
    "daoutlieriaoindcalendaricaindadjustfaciafindadjustfacpctipfindcratio"
    "criindrratiorriindforcefactoriff";

static const int SAV_tb3ptr[163] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 8, 11, 21, 24, 33, 36, 48, 51, 58, 61, 71, 74, 84, 87, 100, 103, 110,
    113, 125, 128, 145, 148, 163, 165, 165, 165, 185, 188, 207, 210, 210,
    210, 210, 210, 220, 223, 234, 237, 248, 251, 265, 268, 283, 286, 296,
    299, 307, 310, 322, 325, 340, 343, 357, 360, 370, 373, 382, 385, 396,
    399, 413, 416, 428, 431, 446, 449, 464, 467, 485, 488, 503, 506, 524,
    527, 542, 545, 563, 566, 587, 590, 614, 617, 628, 631, 649, 652, 670,
    673, 685, 688, 688, 688, 688, 688, 699, 702, 702, 702, 702, 702, 702,
    702, 713, 716, 728, 731, 731, 731, 731, 731, 731, 731, 731, 731, 731,
    731, 731, 731, 731, 731, 731, 731, 744, 747, 758, 761, 772, 775, 787,
    790, 805, 808, 817, 820, 829, 832, 846, 849
};

static const char SAV_TB4DIC[] =
    "trends12trendconststcseasonals10seasonalpctpssirregulars13irregularp"
    "ctpsiseasonaladjs11seasadjconstsectransitorys14transitorypctpscadjus"
    "tfacs16adjustfacpctpsatrendfcstdecomptfdseasonalfcstdecompsfdseriesf"
    "cstdecompofdseasonaladjfcstdecompafdtransitoryfcstdecompyfdadjustmen"
    "tratios18totaladjustmentstawkendfilterwkfcomponentmodelsmdcpseudoinn"
    "ovtrendpicpseudoinnovseasonalpispseudoinnovtransitorypitpseudoinnovs"
    "adjpiasquaredgainsasymgafsquaredgainsaconcgacsquaredgaintrendsymgtfs"
    "quaredgaintrendconcgtctimeshiftsaconctactimeshifttrendconcttcfilters"
    "asymfaffiltersaconcfacfiltertrendsymftffiltertrendconcftcdifforigina"
    "ldordiffseasonaladjdsadifftrenddtrseasonalsumssmcyclecyclongtermtren"
    "dlttseasonalsesseseasonaladjseasetrendsetsetransitorysecseseasonalad"
    "joutlieradjse2irregularoutlieradjse3trendadjlsstl";

static const int SAV_tb4ptr[97] = {
    1, 6, 9, 19, 22, 30, 33, 44, 47, 56, 59, 71, 74, 85, 88, 100, 103, 113,
    116, 129, 132, 141, 144, 156, 159, 174, 177, 195, 198, 214, 217, 238,
    241, 261, 264, 279, 282, 297, 300, 311, 314, 329, 332, 348, 351, 370,
    373, 394, 397, 412, 415, 431, 434, 451, 454, 473, 476, 496, 499, 514,
    517, 535, 538, 549, 552, 564, 567, 581, 584, 599, 602, 614, 617, 632,
    635, 644, 647, 658, 661, 666, 669, 682, 685, 695, 698, 711, 714, 721,
    724, 736, 739, 760, 763, 782, 785, 795, 798
};

// getprt.f:75-91 / getsav.f:35-46 -- the four-way dispatch. Spcdsp is a TABLE
// displacement; each dictionary's pointer array is indexed from its own base,
// hence the subtraction.
struct DicSlice { const char* dic; const int* ptr; int base; };

DicSlice pick(bool save, int lsp) {
    if (lsp < BRKDSP)
        return save ? DicSlice{SAV_TB1DIC, SAV_tb1ptr, lsp}
                    : DicSlice{PRT_TB1DIC, PRT_tb1ptr, lsp};
    if (lsp < BRKDS2)
        return save ? DicSlice{SAV_TB2DIC, SAV_tb2ptr, lsp - BRKDSP}
                    : DicSlice{PRT_TB2DIC, PRT_tb2ptr, lsp - BRKDSP};
    if (lsp < BRKDS3)
        return save ? DicSlice{SAV_TB3DIC, SAV_tb3ptr, lsp - BRKDS2}
                    : DicSlice{PRT_TB3DIC, PRT_tb3ptr, lsp - BRKDS2};
    return save ? DicSlice{SAV_TB4DIC, SAV_tb4ptr, lsp - BRKDS3}
                : DicSlice{PRT_TB4DIC, PRT_tb4ptr, lsp - BRKDS3};
}

}  // namespace

// Returns the 1-based index WITHIN the spec's slice, or 0 when the current
// token is not a name in it. The `Prttab`/`Savtab`/`tblmsk` stores stay
// deferred with the rest of table selection; only the lookup is ported, which
// is the half that decides OUTCOME.
int tbldic_lookup(X13Context& ctx, bool save, int lsp, int nsp) {
    const DicSlice s = pick(save, lsp);
    int idx = 0;
    bool argok = true;
    gtdcnm(ctx, s.dic, &s.ptr[2 * s.base], 2 * nsp, idx, argok);
    return idx;
}

}  // namespace x13
