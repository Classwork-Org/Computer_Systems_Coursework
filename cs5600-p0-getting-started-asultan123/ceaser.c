#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define ALPHABET_SIZE 26

int encode_letter(char letter, int key)
{
    // adjust key if it's magnitude exceeds ALPAHBET_SIZE
    key = key % ALPHABET_SIZE;
    int ascii_position = (int)letter - (int)'A';

    if (key > 0)
    {
        // determine wrap around beyond end of alphabet
        if (ascii_position + key >= ALPHABET_SIZE)
        {
            return (char)((int)letter + (key - ALPHABET_SIZE));
        }
        else
        {
            return (char)((int)letter + key);
        }
    }
    else
    {
        // determine wrap around before begining of alphabet
        if (ascii_position + key < 0)
        {
            return (char)((int)letter + (key + ALPHABET_SIZE));
        }
        else
        {
            return (char)((int)letter + key);
        }
    }
}

char *encode(const char *plaintext, int key)
{
    int idx;

    //allocate result pointer and set to all 0's
    char *result = (char *)malloc(sizeof(char) * strlen(plaintext));
    if (result == NULL)
    {
        return NULL;
    }
    memset(result, 0, strlen(plaintext));

    //encode each character if it's alphanumeric
    for (idx = 0; idx < strlen(plaintext); idx++)
    {
        //to upper does not affect non-alphanumeric characters
        result[idx] = toupper(plaintext[idx]);
        if (isalpha(plaintext[idx]))
        {
            result[idx] = encode_letter(result[idx], key);
        }
    }

    return result;
}

//decode is just encode with key sign flipped if encode is shifting plaintext
//letters down or up by a certain value, decode is an encode in the opposite
//direction
char *decode(const char *cypher_text, int key)
{
    char *result = encode(cypher_text, -1 * key);

    return result;
}

int main(int argc, char const *argv[])
{
    char plain_text[] = "hello world!";

    //using negative magintude > ALPHABSET SIZE example
    //-27 shift is equivelent to a leftwards shift in the alphabet of -1
    char *cypher_text = encode(plain_text, -27);
    if (cypher_text == NULL)
    {
        printf("FAILED TO ALLOCATE MEMORY FOR CYPHER TEXT!\n");
    }

    char *decoded_text = decode(cypher_text, -27);
    if (decoded_text == NULL)
    {
        printf("FAILED TO ALLOCATE MEMORY FOR DECODED TEXT!\n");
    }

    printf("ENCODING STRING: %s \n", plain_text);
    printf("CYPHER STRING: %s \n", cypher_text);
    printf("DECODED CYPHER STRING: %s \n", decoded_text);

    free(cypher_text);
    free(decoded_text);

    return 0;
}
