#ifndef _AK_AUDIO_CONFIG_H_
#include "ak_common_audio.h"
/* set ai param */
struct ak_audio_nr_attr default_ai_nr_attr ={-25, 0, 1};
struct ak_audio_agc_attr default_ai_agc_attr = {24576, 6, 0, 40, 0, 1};
struct ak_audio_aec_attr default_ai_aec_attr = {0, 1024, 1024, 0, 512, 1, 6553};
struct ak_audio_aslc_attr default_ai_aslc_attr = {9830, 0, 0};//limit, volume
struct ak_audio_eq_attr default_ai_eq_attr = {
0,
5,
{500, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000},//EQ Ferq
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},//Band Gain
{717, 717, 717, 717, 717, 717, 717, 717, 717, 717},
{TYPE_HPF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
0,
0,
0,
0,
0,
0,
1,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/* set ao param */
struct ak_audio_nr_attr default_ao_nr_attr = {-25, 0, 1};
struct ak_audio_aslc_attr default_ao_aslc_attr = {9830, 5, 0};///limit, volume
/* gain */
 
int default_ai_gain = 3;
 
/*			### how to set ai gain ###
ak_ai_set_gain(ai_handle_id,default_ai_gain);*/
 
int default_ao_gain = 3;
 
/*			### how to set ao gain ###
ak_ao_set_gain(ao_handle_id,default_ao_gain);*/
 
struct ak_audio_eq_attr default_ao_eq_attr = {
0,
5,
{1000, 3000, 4000, 1000, 1250, 1000, 2000, 4000, 8000, 16000},
{0, -12288, 0, -12288, -8192, 0, 0, 0, 0, 0},
{716, 716, 716, 716, 716, 717, 717, 717, 717, 717},
{TYPE_HPF, TYPE_LPF, TYPE_LPF, TYPE_LSF, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1, TYPE_PF1},
0,
0,
0,
0,
0,
0,
1,
{1, 1, 1, 1, 1, 0, 0, 0, 0, 0}
};
#endif
