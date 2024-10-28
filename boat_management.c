#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BOATS 120
#define MAX_NAME_LENGTH 128
#define MAX_trailor_TAG_LENGTH 10

// Enum for place type
typedef enum { slip, land, trailor, storage, no_place } PlaceType;

// Union for additional place info
typedef union {
  int slipNumber;                          // for slip
  char bayLetter;                          // for land
  char trailorTag[MAX_trailor_TAG_LENGTH]; // for trailor
  int storageNumber;                       // for storage
} PlaceInfo;

// Struct for boat
typedef struct {
  char name[MAX_NAME_LENGTH];
  int length;        // length in feet
  PlaceType place;   // type of place
  PlaceInfo info;    // additional place information
  double amountOwed; // amount owed
} Boat;

// Function prototypes
void loadData(Boat *boats[], int *boatCount, const char *filename);
void saveData(const char *filename, Boat *boats[], int boatCount);
void displayInventory(Boat *boats[], int boatCount);
void addBoat(Boat *boats[], int *boatCount);
void removeBoat(Boat *boats[], int *boatCount);
void acceptPayment(Boat *boats[], int boatCount);
void updateMonthlyCharges(Boat *boats[], int boatCount);
PlaceType stringToPlaceType(const char *placeString);
void toLowerCase(char *str);
int validateBoatInfo(Boat *boat);
int validateLength(int length);
void sortBoats(Boat *boats[], int boatCount);
int findBoatByName(Boat *boats[], int boatCount, const char *name);
int compareBoats(const void *a, const void *b);

// Main function
int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  Boat *boats[MAX_BOATS] = {NULL};
  int boatCount = 0;

  // Load data from file
  loadData(boats, &boatCount, argv[1]);

  printf("\nWelcome to the Boat Management System\n");
  printf("-------------------------------------\n");

  char option;
  do {
    printf("\n");
    printf("(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
    if (scanf(" %c", &option) != 1) { // Check return value of scanf
      printf("Error reading input.\n");
      continue;
    }
    toLowerCase(&option);

    switch (option) {
    case 'i':
      displayInventory(boats, boatCount);
      break;
    case 'a':
      addBoat(boats, &boatCount);
      break;
    case 'r':
      removeBoat(boats, &boatCount);
      break;
    case 'p':
      acceptPayment(boats, boatCount);
      break;
    case 'm':
      updateMonthlyCharges(boats, boatCount);
      break;
    case 'x':
      printf("Exiting the Boat Management System\n");
      saveData(argv[1], boats, boatCount); // Save data to file
      break;
    default:
      printf("Invalid option %c\n", option);
    }
  } while (option != 'x');

  // Free allocated memory
  for (int i = 0; i < boatCount; i++) {
    free(boats[i]);
  }

  return 0;
}

// Load data from CSV file with validation
void loadData(Boat *boats[], int *boatCount, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Error opening file %s for reading.\n", filename); // Error message
    return;
  }

  char line[256];
  while (fgets(line, sizeof(line), file)) { 
    Boat *boat = malloc(sizeof(Boat)); // Allocate memory for boat
    if (!boat) {
      printf("Memory allocation failed.\n");
      fclose(file);
      return;
    }

    char placeString[20], info[20];
    if (sscanf(line, "%127[^,],%d,%19[^,],%19[^,],%lf", boat->name, 
               &boat->length, placeString, info, &boat->amountOwed) == 5) { 
      // Validate length
      if (!validateLength(boat->length)) {
        printf("Invalid boat length: %d. Must be between 1 and 100.\n",
               boat->length);
        free(boat);
        continue;
      }

      boat->place = stringToPlaceType(placeString);
      if (boat->place == no_place) { // if invalid place type
        printf("Invalid place type '%s' in file.\n", placeString);
        free(boat);
        continue;
      }

      // Assign additional info based on place type
      switch (boat->place) {
      case slip:
        boat->info.slipNumber = atoi(info);
        break;
      case land:
        boat->info.bayLetter = info[0];
        break;
      case trailor:
        strncpy(boat->info.trailorTag, info, MAX_trailor_TAG_LENGTH);
        boat->info.trailorTag[MAX_trailor_TAG_LENGTH - 1] =
            '\0'; // Ensure null termination
        break;
      case storage:
        boat->info.storageNumber = atoi(info);
        break;
      default:
        free(boat);
        continue;
      }
      // Validate place-specific information
      if (!validateBoatInfo(boat)) {
        free(boat);
        continue;
      }

      boats[*boatCount] = boat;
      (*boatCount)++;
    } else {
      printf("Error parsing line: %s", line);
      free(boat);
    }
  }

  fclose(file);
  sortBoats(boats, *boatCount);
}

// Save data to CSV file with error handling for writes
void saveData(const char *filename, Boat *boats[], int boatCount) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    printf("Error opening file %s for writing.\n", filename);
    return;
  }
  // Write data for each boat
  for (int i = 0; i < boatCount; i++) {
    Boat *boat = boats[i];
    int writeStatus = 0;

    switch (boat->place) {
    case slip:
      writeStatus =
          fprintf(file, "%s,%d,slip,%d,%.2f\n", boat->name, boat->length,
                  boat->info.slipNumber, boat->amountOwed);
      break;
    case land:
      writeStatus =
          fprintf(file, "%s,%d,land,%c,%.2f\n", boat->name, boat->length,
                  boat->info.bayLetter, boat->amountOwed);
      break;
    case trailor:
      writeStatus =
          fprintf(file, "%s,%d,trailor,%s,%.2f\n", boat->name, boat->length,
                  boat->info.trailorTag, boat->amountOwed);
      break;
    case storage:
      writeStatus =
          fprintf(file, "%s,%d,storage,%d,%.2f\n", boat->name, boat->length,
                  boat->info.storageNumber, boat->amountOwed);
      break;
    default:
      break;
    }

    if (writeStatus < 0) {
      printf("Error writing data for boat: %s\n", boat->name);
      fclose(file);
      return;
    }
  }

  fclose(file);
  printf("Data saved successfully to %s\n",
         filename); // Confirm save completion
}

// Display boat inventory
void displayInventory(Boat *boats[], int boatCount) {
  printf("\nBoat Inventory:\n");
  for (int i = 0; i < boatCount; i++) {
    Boat *boat = boats[i];
    printf("%-20s %3d' ", boat->name, boat->length); 
    switch (boat->place) {
    case slip:
      printf("   slip   # %2d   ", boat->info.slipNumber);
      break;
    case land:
      printf("   land      %c   ", boat->info.bayLetter);
      break;
    case trailor:
      printf("trailor %s   ", boat->info.trailorTag);
      break;
    case storage:
      printf("storage # %2d   ", boat->info.storageNumber);
      break;
    default:
      printf("unknown       ");
      break;
    }
    printf("Owes $%8.2f\n", boat->amountOwed);
  }
}

// Validate boat length
int validateLength(int length) { return length >= 1 && length <= 100; }

// Add a new boat with enhanced validation
void addBoat(Boat *boats[], int *boatCount) {
  if (*boatCount >= MAX_BOATS) {
    printf("Cannot add more boats, maximum capacity reached.\n");
    return;
  }
  // Allocate memory for a new boat
  Boat *boat = malloc(sizeof(Boat));
  if (!boat) {
    printf("Memory allocation failed.\n");
    return;
  }

  printf("Enter the boat data in CSV format (name,length,place,info,amount): ");
  char line[256];
  if (scanf(" %[^\n]", line) != 1) { // Check return value of scanf
    printf("Error reading input.\n");
    free(boat);
    return;
  }

  char placeString[20], info[20];
  // Parse input line
  if (sscanf(line, "%127[^,],%d,%19[^,],%19[^,],%lf", boat->name, &boat->length,
             placeString, info, &boat->amountOwed) != 5) {
    printf("Error: Invalid input format.\n");
    free(boat);
    return;
  }

  // Validate boat length
  if (!validateLength(boat->length)) {
    printf("Invalid boat length: %d. Must be between 1 and 100.\n",
           boat->length);
    free(boat);
    return;
  }

  // Check if the boat name already exists
  if (findBoatByName(boats, *boatCount, boat->name) != -1) {
    printf("Boat with name '%s' already exists.\n", boat->name);
    free(boat);
    return;
  }

  // Set place type and validate extra info based on place type
  boat->place = stringToPlaceType(placeString);
  switch (boat->place) {
  case slip:
    // Convert info to slip number and validate it
    boat->info.slipNumber = atoi(info);
    if (boat->info.slipNumber < 1 || boat->info.slipNumber > 85) {
      printf("Invalid slip number. Must be between 1 and 85.\n");
      free(boat);
      return;
    }
    break;
  case land:
    // Assign bay letter and validate it
    boat->info.bayLetter = info[0];
    if (boat->info.bayLetter < 'A' || boat->info.bayLetter > 'Z') {
      printf("Invalid bay letter. Must be between A and Z.\n");
      free(boat);
      return;
    }
    break;
  case trailor:
    // Copy trailor tag
    strncpy(boat->info.trailorTag, info, MAX_trailor_TAG_LENGTH);
    boat->info.trailorTag[MAX_trailor_TAG_LENGTH - 1] =
        '\0'; // Ensure null termination
    break;
  case storage:
    boat->info.storageNumber = atoi(info);
    if (boat->info.storageNumber < 1 || boat->info.storageNumber > 50) {
      printf("Invalid storage number. Must be between 1 and 50.\n");
      free(boat);
      return;
    }
    break;
  default:
    printf("Invalid place type.\n");
    free(boat);
    return;
  }

  // Add boat to the array
  boats[*boatCount] = boat;
  (*boatCount)++;
  // Sort the boats array after adding the new boat
  sortBoats(boats, *boatCount);
  printf("Boat added successfully!\n");
}

// Accept payment for a boat
void acceptPayment(Boat *boats[], int boatCount) {
  printf("Please enter the boat name: ");
  char name[MAX_NAME_LENGTH];
  if (scanf(" %[^\n]", name) != 1) { // Check return value of scanf
    printf("Error reading input.\n");
    return;
  }
  // Find the boat by name in the array of boats
  int index = findBoatByName(boats, boatCount, name);
  if (index == -1) {
    printf("No boat with name '%s' found.\n", name);
    return;
  }

  Boat *boat = boats[index];
  double payment;
  printf("Enter the payment amount: ");
  if (scanf("%lf", &payment) != 1) { // Check return value of scanf
    printf("Error reading payment amount.\n");
    return;
  }
  // Check if the payment exceeds the amount owed
  if (payment > boat->amountOwed) {
    printf("Payment exceeds amount owed of $%.2f.\n", boat->amountOwed);
    return;
  }
  // Deduct the payment from the amount owed and print the new balance
  boat->amountOwed -= payment;
  printf("Payment accepted. New balance: $%.2f\n", boat->amountOwed);
}

void removeBoat(Boat *boats[], int *boatCount) {
  printf("Please enter the boat name to remove: ");
  char name[MAX_NAME_LENGTH];
  if (scanf(" %[^\n]", name) != 1) { // Check return value of scanf
    printf("Error reading boat name.\n");
    return;
  }
  // Find the boat by name in the array of boats
  int index = findBoatByName(boats, *boatCount, name);
  if (index == -1) {
    printf("No boat with name '%s' found.\n", name);
    return;
  }
  // Free the memory allocated for the boat
  free(boats[index]);
  // Shift the remaining boats in the array to fill the gap
  for (int i = index; i < *boatCount - 1; i++) {
    boats[i] = boats[i + 1];
  }
  // Decrease the boat count
  (*boatCount)--;
  // Sort the boats array after removal
  sortBoats(boats, *boatCount);
  printf("Boat removed successfully!\n");
}

// Update monthly charges for all boats
void updateMonthlyCharges(Boat *boats[], int boatCount) {
  for (int i = 0; i < boatCount; i++) {
    switch (boats[i]->place) {
    // Add monthly charges based on place type
    case slip:
      boats[i]->amountOwed += boats[i]->length * 12.50;
      break;
    case land:
      boats[i]->amountOwed += boats[i]->length * 14.00;
      break;
    case trailor:
      boats[i]->amountOwed += boats[i]->length * 25.00;
      break;
    case storage:
      boats[i]->amountOwed += boats[i]->length * 11.20;
      break;
    default:
      break;
    }
  }
  printf("Monthly charges updated for all boats.\n");
}

// Convert string to PlaceType
PlaceType stringToPlaceType(const char *placeString) {
  if (strcasecmp(placeString, "slip") == 0)
    return slip;
  if (strcasecmp(placeString, "land") == 0)
    return land;
  if (strcasecmp(placeString, "trailor") == 0)
    return trailor;
  if (strcasecmp(placeString, "storage") == 0)
    return storage;

  printf("Unknown place type: %s\n",
         placeString); // Debugging message for invalid place type
  return no_place;
}

// Convert input to lowercase for case-insensitive comparison
void toLowerCase(char *str) {
  for (int i = 0; str[i]; i++) {
    str[i] = tolower(str[i]);
  }
}

// Validate place-specific information based on type
int validateBoatInfo(Boat *boat) {
  switch (boat->place) {
  case slip:
    // Validate slip number is between 1 and 85
    return boat->info.slipNumber >= 1 && boat->info.slipNumber <= 85;
  case land:
    // Validate bay letter is between 'A' and 'Z'
    return boat->info.bayLetter >= 'A' && boat->info.bayLetter <= 'Z';
  case storage:
    // Validate storage number is between 1 and 50
    return boat->info.storageNumber >= 1 && boat->info.storageNumber <= 50;
  case trailor:
    return 1; // No specific validation needed for trailor tag
  default:
    return 0;
  }
}

// Find a boat by name (case-insensitive)
int findBoatByName(Boat *boats[], int boatCount, const char *name) {
  for (int i = 0; i < boatCount; i++) {
    if (strcasecmp(boats[i]->name, name) == 0) {
      return i;
    }
  }
  return -1;
}

// Compare two boats by name for sorting
int compareBoats(const void *a, const void *b) {
  Boat *boatA = *(Boat **)a;
  Boat *boatB = *(Boat **)b;
  return strcasecmp(boatA->name, boatB->name);
}

// Sort boats array alphabetically by name
void sortBoats(Boat *boats[], int boatCount) {
  qsort(boats, boatCount, sizeof(Boat *), compareBoats);
}