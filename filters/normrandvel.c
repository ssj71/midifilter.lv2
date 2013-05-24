MFD_FILTER(normrandvel)

#ifdef MX_TTF

    mflt:normrandvel
    TTF_DEFAULTDEF("MIDI Velocity Randomization")
    , TTF_IPORT(0, "channel", "Channel", 0.0, 16.0, 0.0, \
    lv2:portProperty lv2:integer; lv2:scalePoint [ rdfs:label "Any" ; rdf:value 0.0 ])
    , TTF_IPORTFLOAT(1, "dev", "Velocity Standard Deviation", 0.0, 64.0, 8.0)
    .

#elif defined MX_CODE

float V;

static void filter_init_normrandvel(MidiFilter* self) {
    srandom ((unsigned int) time (NULL));
    V = 2*random() / (float)RAND_MAX - 1;
}


//calculates random numbers according to a modified Marsaglia polar method,
// approaching a normal distribution, should be more "human"
// User sets standard dev.
static void
filter_midi_normrandvel(MidiFilter* self,
        uint32_t tme,
        const uint8_t* const buffer,
        uint32_t size)
{
    const int chs = midi_limit_chn(floor(*self->cfg[0]) -1);
    const int chn = buffer[0] & 0x0f;
    const int msg = buffer[0] & 0xf0;

    if (size == 3
            && (msg == 0x90 || msg == 0x80) // note on, off
            && (floor(*self->cfg[0]) == 0 || chs == chn)
            )
    {
        uint8_t buf[3];
        const float dev = *(self->cfg[1]);
        float U = 2.0* random() / (float)RAND_MAX - 1;//rand E(-1,1)
        float S = U*U + V*V;//map 2 random vars to unit circle
        uint8_t timeout = 0;
        while(S>=1)//repull RV if outside unit circlehttp://pastebin.com/cEzJLZXh
        {   
            if(timeout++>2)
            {
                U = 2.0* random() / (float)RAND_MAX - 1;
                S = U*U + V*V;
            }else{//guarantee an exit, velocity will be unchanged
                U=0;
            }
        }
        const float rnd = dev*U*sqrt(-2*log(S)/S);
        V = U;//store RV for next round
        buf[0] = buffer[0];
        buf[1] = buffer[1];
        buf[2] = midi_limit_val(rintf(buffer[2] + rnd));
        forge_midimessage(self, tme, buf, size);
    } else {
        forge_midimessage(self, tme, buffer, size);
    }
}

#endif
