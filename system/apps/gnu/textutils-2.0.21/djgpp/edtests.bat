@echo off
echo Editing test scripts in tests/ subdirectories for DJGPP...
echo    tests/cut...
if not exist tests\cut\cut-tests.orig mv -f tests/cut/cut-tests tests/cut/cut-tests.orig
sed "s/missing-/miss-/g" tests/cut/cut-tests.orig > tests\cut\cut-tests
echo    tests/head...
if not exist tests\head\head-tests.orig mv -f tests/head/head-tests tests/head/head-tests.orig
sed "s/basic-0/bas-0/g" tests/head/head-tests.orig > tests\head\head-tests
echo    tests/pr...
if not exist tests\pr\pr-tests.orig mv -f tests/pr/pr-tests tests/pr/pr-tests.orig
sed -f djgpp/tscript.sed tests/pr/pr-tests.orig > tests\pr\pr-tests
rem
rem Need to convert expected test output files to DOS text format
rem for those programs which write stdout in text mode.  Otherwise
rem cmp will report differences and we will think the tests failed.
rem
echo Converting expected-output files to DOS text format...
utod tests/cut/*.X tests/join/*.X tests/wc/*.X
utod tests/pr/*.X tests/sort/*.X tests/uniq/*.X
cd tests\pr
utod 0F 3-0F 3f-0F a3-0F a3f-0F 3a3f-0F b3-0F b3f-0F 3b3f-0F 0FF a3f-0FF
utod b3f-0FF 3b3f-0FF FF 3-FF 3f-FF a3f-FF b3f-FF 3b3f-FF l24-FF
utod a2l17-FF a2l15-FF b2l17-FF b2l15-FF 4l24-FF 4-7l24-FF 3a2l17-FF 3b2l17-FF
utod l24-t l17f-t 3l24-t 3l17f-t 3-5l17f-t a3l15-t a3l8f-t 3a3l15-t 3a3l8f-t
utod b3l15-t b3l8f-t 3b3l15-t 3b3l8f-t ml24-t ml17f-t 3ml24-t 3ml17f-t ml17f-0F
utod ml17f-t-0F ml24-FF ml24-t-FF ml20-FF-t 3ml24-FF 3ml24-t-FF t-t t-bl t-0FF
utod t-FF ta3-0FF ta3-FF tb3-0FF tb3-FF tt-t tt-bl tt-0FF tt-FF tta3-0FF
utod tta3-FF ttb3-0FF ttb3-FF nl17f-bl nN15l17f-bl nx2l17f.bl
utod nx3l17f.bl nN1x3l17f-bl nx2l17f.0FF nx2-5l17f-0FF nx3l17f.0FF nx7l24-FF
utod nx3-7l24-FF nx8l20-FF nx5a3l6f-0FF nx6a2l17-FF nx4-8a2l17-FF
utod nx4b2l10f-0FF nx6b3l6f-FF nx5-8b3l10f-FF nx3ml13f-bl.FF nx3ml17f-bl.tn
utod nx3ml17f-tn.bl W20l17f-ll W26l17f-ll W27l17f-ll W28l17f-ll
utod ml17f-lm-lo W35ml17f-lm-lo Jml17f-lm-lo W35Jml17f-lmlo nJml17f-lmlm.lo
utod nJml17f-lmlo.lm a3l17f-lm W35a3l17f-lm Ja3l17f-lm W35Ja3l17f-lm
utod b3l17f-lm W35b3l17f-lm Jb3l17f-lm W35Jb3l17f-lm nSml13-bl-FF nSml17-bl-FF
utod nSml13-t-t-FF nSml17-t-t-FF nSml13-t-tFF.FF nSml17-t-tFF.FF o3a3l17f-tn
utod o3a3Snl17f-tn o3a3Sl17f-tn o3b3l17f-tn o3b3Sl17f-tn o3b3Snl17f-tn
utod o3ml17f-bl-tn o3mSl17f-bl-tn o3mSnl17fbltn o3Jml17f-lm-lo
utod W72Jl17f-ll W72l17f-ll _w72l17f-ll
utod tne8-t_tab tne8o3-t_tab tn2e8-t_tab tn_2e8-t_tab tn_2e8S-t_tab
utod tn2e8o3-t_tab tn2e5o3-t_tab 2f-t_notab 2sf-t_notab 2s_f-t_notab
utod 2w60f-t_notab 2sw60f-t_notab 2s_w60f-t_nota 2_Sf-t_notab 2_S_f-t_notab
cd ..
cd ..
