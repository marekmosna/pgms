#include <postgres.h>
#include <fmgr.h>
#include <common/shortest_dec.h>
#include <errno.h>


PG_FUNCTION_INFO_V1(spectrum_input);
Datum spectrum_input(PG_FUNCTION_ARGS)
{
    char *data = PG_GETARG_CSTRING(0);

    size_t count = 0;

    for(char *buffer = data; *buffer != '\0'; buffer++)
        if(*buffer == ':')
            count++;

    size_t size = count * 2* sizeof(float4) + VARHDRSZ;
    void *result = palloc(size);
    SET_VARSIZE(result, size);

    float4 *values = (float4 *) VARDATA(result);
    char *buffer = data;

    for(size_t i = 0; i < count; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            char *end;
            errno = 0;
            values[j * count + i] = strtof(buffer, &end);

            if(errno != 0 || *end != (j == 0 ? ':' : i == count - 1 ? '\0' : ','))
                ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("malformed spectrum literal")));

            buffer = end + 1;
        }
    }

    PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(spectrum_output);
Datum spectrum_output(PG_FUNCTION_ARGS)
{
    void *spectrum = PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    size_t count = (VARSIZE(spectrum) - VARHDRSZ) / sizeof(float4) / 2;
    float4 *values = (float4 *) VARDATA(spectrum);

    char *result = (char *) palloc(count ? count * (FLOAT_SHORTEST_DECIMAL_LEN + 2) : 1);
    char *buffer = result;

    for(size_t i = 0; i < count; i++)
    {
        buffer += float_to_shortest_decimal_bufn(values[i], buffer);
        *(buffer++) = ':';
        buffer += float_to_shortest_decimal_bufn(values[count + i], buffer);
        *(buffer++) = ',';
    }

    *(count ? buffer - 1 : buffer) = '\0';

    PG_RETURN_CSTRING(result);
    PG_FREE_IF_COPY(spectrum, 0);
}
