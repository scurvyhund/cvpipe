   /* str-mul.c v1.01
    *
    * Program calcs product of 2 values given as arguments on the 
    * cmdln using the simple school book algorithm.
    *
    * Currently gives correct results for arbitrarily large uints.
    *
    * What it does:
    *    1) takes two cmdln args, ea. representing a positive uint
    *       and converts the args to integers
    *    2) calculates the product
    *    3) converts the product to printable str chars and prints
    *       to stdout
    *
    * Example:
    *    $> ./str-mul 56011910234567543 23487759087659433455430567
    * result:
    *    1315594253629128237124760856465318108286881
    *
    * compile:
    *    gcc -Wall -std=c99 -O1 -o str-mul str-mul.c
    */

   #define  _GNU_SOURCE 

   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <stdint.h>

   #define  MAXSTR  80     // max arg[x] len in char digits
   
   void conv_cmdlnArg(char* , char*);
   void array_mult(const char*, const char*, int16_t, int16_t);

   int main(int argc, char* argv[]){

      if(argc < 3) {
         puts("\nCOMMAND LINE ARGS ERROR");
         puts("\nbig-mul uses 2 spc separated strs that ea. represent\t\
               unsigned decimal digit values...");
         puts("\nEg.\n\t $> big-mul 12345 56789");
         puts("Output:\n\t    701060205\n");
         return 0;
      }

      int16_t len_1 = strnlen(argv[1], MAXSTR);
      char* array_1;
      array_1 = (char*)calloc(len_1, sizeof(char));
      
      conv_cmdlnArg(argv[1], array_1);

      // set array_2 to multiplier...
      int16_t len_2 = strnlen(argv[2], MAXSTR);
      char* array_2;
      array_2 = (char*)calloc(len_2, sizeof(char));

      conv_cmdlnArg(argv[2], array_2);

      // Do the math.
      array_mult(array_1, array_2, len_1, len_2);
     
      free(array_1);
      free(array_2);
      
      return 0;
   }

   // parse argv[] dec str chars, conv to integers and load arrays...
   void conv_cmdlnArg(char* arg, char* arr) {
      int8_t len = strnlen(arg, MAXSTR);

      for(int8_t i = len-1; i >= 0; i--)
         arr[i] = arg[i] - '0';
   }

   void array_mult(const char* array_1, const char* array_2,
                   int16_t array_1_len, int16_t array_2_len) {

      // Calc max product len and create an arr. to hold it.
      int16_t product_len = array_1_len + array_2_len;
      
      char* product_array;
      product_array = (char*)calloc(product_len, sizeof(char));

      int8_t carry = 0; uint8_t tmp; int8_t prod_digit_col = 0;

      // set 'j' index to ones' digit in multiplier...
      for(int8_t j = array_2_len - 1; j >= 0; j--) {

         /* As prod_digit_col incs 'k' decs. This indexes columns of success-
          * ively higher magnitude for the addition of partial results each 
          * iter. of 'j' over array_1 in the while loop.
          */
         int16_t k = product_len - 1 - prod_digit_col++;

         // set 'i' index to ones digit in multiplicand...
         int8_t i = array_1_len - 1;

         // iterate over all of array_1 successively with ea. array_2 digit...
         while(i >= 0 || carry > 0) {

            // tmp can be > 10 at any time time.
            if(i >= 0)
               tmp = array_1[i] * array_2[j];
            else 
               tmp = 0;

            // If carry from previous iter. add it now.
            tmp += carry;

            // 0 unless tmp is > than 9.
            carry = tmp/10;

            // adds col's sum  value to k
            product_array[k] += (tmp % 10);

            /* if prev instr. made k > 10, calc the 10's digit
             * out to cy.
             */
            carry += (product_array[k] / 10);
            
            // k has the 10's taken out here.
            product_array[k] = product_array[k] % 10;

            i--; k--;
         }
         // next multiplicand (array_2) digit...
      }
      
      int16_t len_prod = 0;

      // Remove any leading zeros from product_array.
      while(product_array[len_prod] == 0)
         len_prod++;
      
      puts("\n");
      
      for(;  len_prod < product_len; len_prod++) 
         printf("%d", product_array[len_prod]);
      
      free(product_array);
      puts("\n");
   }
