#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ─── Struct ────────────────────────────────────────────────────────────────────
struct clientData
{
    unsigned int acctNum;   // account number
    char lastName[15];      // account last name
    char firstName[10];     // account first name
    double balance;         // account balance
    unsigned int pin;       // 4-digit PIN              ← NEW
    int locked;             // 1 = locked               ← NEW
    int failedAttempts;     // wrong PIN counter         ← NEW
};

// ─── Prototypes ────────────────────────────────────────────────────────────────
unsigned int enterChoice(void);
void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);
void viewAccount(FILE *fPtr);
void viewAllAccounts(FILE *fPtr);
void transferFunds(FILE *fPtr);
void logTransaction(const char *description);
int  verifyPIN(FILE *fPtr, unsigned int account, struct clientData *client);
void setPIN(FILE *fPtr, unsigned int account, struct clientData *client);
void changePin(FILE *fPtr);
void unlockAccount(FILE *fPtr);

// ─── Logging ───────────────────────────────────────────────────────────────────
void logTransaction(const char *description)
{
    FILE *logPtr = fopen("transactions.log", "a");
    if (logPtr == NULL) { puts("Warning: Could not open transaction log."); return; }

    time_t now = time(NULL);
    char timeStr[26];
    strncpy(timeStr, ctime(&now), 25);
    timeStr[24] = '\0';

    fprintf(logPtr, "[%s] %s\n", timeStr, description);
    fclose(logPtr);
}

// ─── PIN: verify ──────────────────────────────────────────────────────────────
// Reads account from disk, verifies PIN, handles lockout.
// Returns 1 on success, 0 on failure / locked.
// Fills *client with the live record on success.
int verifyPIN(FILE *fPtr, unsigned int account, struct clientData *client)
{
    char logMsg[128];

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(client, sizeof(struct clientData), 1, fPtr);

    if (client->acctNum == 0)
    {
        printf("Account #%d does not exist.\n", account);
        return 0;
    }

    // Already locked?
    if (client->locked)
    {
        printf("Account #%d is LOCKED due to too many failed PIN attempts.\n", account);
        puts("Contact admin (menu option 9) to unlock.");
        return 0;
    }

    // No PIN set yet (new account edge-case)
    if (client->pin == 0)
    {
        puts("No PIN set for this account. Please set one now.");
        setPIN(fPtr, account, client);
        return 1;
    }

    // Allow up to 3 attempts
    unsigned int entered;
    int attempt;
    for (attempt = 1; attempt <= 3; attempt++)
    {
        printf("Enter PIN for account #%d (attempt %d/3): ", account, attempt);
        scanf("%u", &entered);

        if (entered == client->pin)
        {
            // Correct — reset counter
            client->failedAttempts = 0;
            fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
            fwrite(client, sizeof(struct clientData), 1, fPtr);
            puts("PIN accepted.");
            return 1;
        }

        printf("Wrong PIN.\n");
    }

    // 3 failures → lock account
    client->failedAttempts = 3;
    client->locked         = 1;
    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(client, sizeof(struct clientData), 1, fPtr);

    printf("Account #%d is now LOCKED after 3 failed attempts.\n", account);

    snprintf(logMsg, sizeof(logMsg),
             "LOCKED Acct#%d %s %s | 3 consecutive wrong PINs",
             client->acctNum, client->firstName, client->lastName);
    logTransaction(logMsg);

    return 0;
}

// ─── PIN: set ─────────────────────────────────────────────────────────────────
void setPIN(FILE *fPtr, unsigned int account, struct clientData *client)
{
    unsigned int pin1, pin2;

    do {
        printf("Set a 4-digit PIN (1000-9999): ");
        scanf("%u", &pin1);
        if (pin1 < 1000 || pin1 > 9999)
            puts("PIN must be exactly 4 digits. Try again.");
    } while (pin1 < 1000 || pin1 > 9999);

    printf("Confirm PIN: ");
    scanf("%u", &pin2);

    if (pin1 != pin2)
    {
        puts("PINs do not match. PIN not changed.");
        return;
    }

    client->pin            = pin1;
    client->locked         = 0;
    client->failedAttempts = 0;

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(client, sizeof(struct clientData), 1, fPtr);

    puts("PIN set successfully.");

    char logMsg[128];
    snprintf(logMsg, sizeof(logMsg), "PIN SET Acct#%d %s %s",
             client->acctNum, client->firstName, client->lastName);
    logTransaction(logMsg);
}

// ─── PIN: change (menu option 8) ─────────────────────────────────────────────
void changePin(FILE *fPtr)
{
    unsigned int account;
    struct clientData client = {0};

    printf("Enter account number to change PIN ( 1 - 100 ): ");
    scanf("%d", &account);
    if (account < 1 || account > 100) { puts("Invalid account number."); return; }

    if (!verifyPIN(fPtr, account, &client)) return;

    puts("Current PIN verified. Set your new PIN:");
    setPIN(fPtr, account, &client);
}

// ─── Unlock account (menu option 9 — admin) ───────────────────────────────────
void unlockAccount(FILE *fPtr)
{
    unsigned int account;
    struct clientData client = {0};
    char logMsg[128];

    printf("Enter account number to unlock ( 1 - 100 ): ");
    scanf("%d", &account);
    if (account < 1 || account > 100) { puts("Invalid account number."); return; }

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)  { printf("Account #%d does not exist.\n", account); return; }
    if (!client.locked)       { printf("Account #%d is not locked.\n", account);  return; }

    client.locked         = 0;
    client.failedAttempts = 0;
    client.pin            = 0;  // force owner to set a new PIN for security

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    printf("Account #%d unlocked. Owner must set a new PIN on next access.\n", account);

    snprintf(logMsg, sizeof(logMsg), "UNLOCKED Acct#%d %s %s | PIN reset by admin",
             client.acctNum, client.firstName, client.lastName);
    logTransaction(logMsg);
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    FILE *cfPtr;
    unsigned int choice;

    if ((cfPtr = fopen("credit.dat", "rb+")) == NULL)
    {
        printf("%s: File could not be opened.\n", argv[0]);
        exit(-1);
    }

    while ((choice = enterChoice()) != 10)
    {
        switch (choice)
        {
        case 1:  textFile(cfPtr);        break;
        case 2:  updateRecord(cfPtr);    break;
        case 3:  newRecord(cfPtr);       break;
        case 4:  deleteRecord(cfPtr);    break;
        case 5:  viewAccount(cfPtr);     break;
        case 6:  viewAllAccounts(cfPtr); break;
        case 7:  transferFunds(cfPtr);   break;
        case 8:  changePin(cfPtr);       break;
        case 9:  unlockAccount(cfPtr);   break;
        default: puts("Incorrect choice"); break;
        }
    }

    fclose(cfPtr);
    puts("Goodbye!");
    return 0;
}

// ─── textFile ─────────────────────────────────────────────────────────────────
void textFile(FILE *readPtr)
{
    FILE *writePtr;
    int result;
    struct clientData client = {0};

    if ((writePtr = fopen("accounts.txt", "w")) == NULL)
    {
        puts("File could not be opened.");
        return;
    }

    rewind(readPtr);
    fprintf(writePtr, "%-6s%-16s%-11s%10s  %s\n",
            "Acct", "Last Name", "First Name", "Balance", "Status");

    while (!feof(readPtr))
    {
        result = fread(&client, sizeof(struct clientData), 1, readPtr);
        if (result != 0 && client.acctNum != 0)
        {
            fprintf(writePtr, "%-6d%-16s%-11s%10.2f  %s\n",
                    client.acctNum, client.lastName, client.firstName,
                    client.balance, client.locked ? "LOCKED" : "Active");
        }
    }

    fclose(writePtr);
    puts("accounts.txt has been created.");
    logTransaction("Exported all accounts to accounts.txt");
}

// ─── updateRecord — PIN protected ────────────────────────────────────────────
void updateRecord(FILE *fPtr)
{
    unsigned int account;
    double transaction;
    struct clientData client = {0};
    char logMsg[128];

    printf("Enter account to update ( 1 - 100 ): ");
    scanf("%d", &account);
    if (account < 1 || account > 100) { puts("Invalid account number."); return; }

    if (!verifyPIN(fPtr, account, &client)) return;   // ← PIN gate

    printf("\n%-6d%-16s%-11s%10.2f\n\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    printf("Enter charge ( + ) or payment ( - ): ");
    scanf("%lf", &transaction);

    double oldBalance  = client.balance;
    client.balance    += transaction;

    printf("Updated: %-6d%-16s%-11s%10.2f\n",
           client.acctNum, client.lastName, client.firstName, client.balance);

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    snprintf(logMsg, sizeof(logMsg),
             "UPDATE Acct#%d %s %s | %.2f -> %.2f (transaction: %.2f)",
             client.acctNum, client.firstName, client.lastName,
             oldBalance, client.balance, transaction);
    logTransaction(logMsg);
}

// ─── deleteRecord — PIN protected ────────────────────────────────────────────
void deleteRecord(FILE *fPtr)
{
    struct clientData client      = {0};
    struct clientData blankClient = {0};
    unsigned int accountNum;
    char logMsg[128];

    printf("Enter account number to delete ( 1 - 100 ): ");
    scanf("%d", &accountNum);
    if (accountNum < 1 || accountNum > 100) { puts("Invalid account number."); return; }

    if (!verifyPIN(fPtr, accountNum, &client)) return;   // ← PIN gate

    snprintf(logMsg, sizeof(logMsg),
             "DELETE Acct#%d %s %s | Balance was %.2f",
             client.acctNum, client.firstName, client.lastName, client.balance);

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&blankClient, sizeof(struct clientData), 1, fPtr);

    printf("Account #%d deleted.\n", accountNum);
    logTransaction(logMsg);
}

// ─── newRecord — sets PIN on creation ────────────────────────────────────────
void newRecord(FILE *fPtr)
{
    struct clientData client = {0};
    unsigned int accountNum;
    char logMsg[128];

    printf("Enter new account number ( 1 - 100 ): ");
    scanf("%d", &accountNum);
    if (accountNum < 1 || accountNum > 100) { puts("Invalid account number."); return; }

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum != 0)
    {
        printf("Account #%d already contains information.\n", client.acctNum);
        return;
    }

    printf("Enter lastname, firstname, balance\n? ");
    scanf("%14s%9s%lf", client.lastName, client.firstName, &client.balance);
    client.acctNum = accountNum;

    // Write record first, then set PIN (setPIN does its own fwrite)
    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    puts("Please set a PIN for this new account.");
    setPIN(fPtr, accountNum, &client);   // ← PIN gate at creation

    printf("Account #%d created successfully.\n", accountNum);

    snprintf(logMsg, sizeof(logMsg),
             "CREATE Acct#%d %s %s | Opening balance: %.2f",
             client.acctNum, client.firstName, client.lastName, client.balance);
    logTransaction(logMsg);
}

// ─── viewAccount ──────────────────────────────────────────────────────────────
void viewAccount(FILE *fPtr)
{
    unsigned int account;
    struct clientData client = {0};

    printf("Enter account number to view ( 1 - 100 ): ");
    scanf("%d", &account);
    if (account < 1 || account > 100) { puts("Invalid account number."); return; }

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0) { printf("Account #%d does not exist.\n", account); return; }

    printf("\n%-6s%-16s%-11s%10s  %s\n", "Acct", "Last Name", "First Name", "Balance", "Status");
    printf("%-6d%-16s%-11s%10.2f  %s\n",
           client.acctNum, client.lastName, client.firstName,
           client.balance, client.locked ? "LOCKED" : "Active");
}

// ─── viewAllAccounts ──────────────────────────────────────────────────────────
void viewAllAccounts(FILE *fPtr)
{
    struct clientData client = {0};
    int result, count = 0;

    rewind(fPtr);
    printf("\n%-6s%-16s%-11s%10s  %s\n", "Acct", "Last Name", "First Name", "Balance", "Status");
    printf("%-6s%-16s%-11s%10s  %s\n",   "----", "---------", "----------", "-------", "------");

    while (!feof(fPtr))
    {
        result = fread(&client, sizeof(struct clientData), 1, fPtr);
        if (result != 0 && client.acctNum != 0)
        {
            printf("%-6d%-16s%-11s%10.2f  %s\n",
                   client.acctNum, client.lastName, client.firstName,
                   client.balance, client.locked ? "LOCKED" : "Active");
            count++;
        }
    }

    if (count == 0) puts("No accounts found.");
    else printf("\nTotal accounts: %d\n", count);
}

// ─── transferFunds — PIN protected on source account ─────────────────────────
void transferFunds(FILE *fPtr)
{
    unsigned int fromAcct, toAcct;
    double amount;
    struct clientData fromClient = {0};
    struct clientData toClient   = {0};
    char logMsg[256];

    printf("Enter source account number ( 1 - 100 ): ");
    scanf("%d", &fromAcct);
    printf("Enter destination account number ( 1 - 100 ): ");
    scanf("%d", &toAcct);

    if (fromAcct == toAcct) { puts("Source and destination cannot be the same."); return; }
    if (fromAcct < 1 || fromAcct > 100 || toAcct < 1 || toAcct > 100)
    {
        puts("Invalid account number(s).");
        return;
    }

    if (!verifyPIN(fPtr, fromAcct, &fromClient)) return;   // ← PIN gate

    fseek(fPtr, (toAcct - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&toClient, sizeof(struct clientData), 1, fPtr);

    if (toClient.acctNum == 0)
    {
        printf("Destination account #%d does not exist.\n", toAcct);
        return;
    }

    printf("\nFrom: %-6d%-16s%-11s Balance: %10.2f\n",
           fromClient.acctNum, fromClient.lastName, fromClient.firstName, fromClient.balance);
    printf("To:   %-6d%-16s%-11s Balance: %10.2f\n",
           toClient.acctNum, toClient.lastName, toClient.firstName, toClient.balance);

    printf("Enter amount to transfer: ");
    scanf("%lf", &amount);

    if (amount <= 0) { puts("Transfer amount must be positive."); return; }
    if (fromClient.balance < amount)
    {
        printf("Insufficient funds. Available: %.2f\n", fromClient.balance);
        return;
    }

    fromClient.balance -= amount;
    toClient.balance   += amount;

    fseek(fPtr, (fromAcct - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&fromClient, sizeof(struct clientData), 1, fPtr);

    fseek(fPtr, (toAcct - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&toClient, sizeof(struct clientData), 1, fPtr);

    printf("\nTransfer successful!\n");
    printf("Acct #%d new balance: %.2f\n", fromClient.acctNum, fromClient.balance);
    printf("Acct #%d new balance: %.2f\n", toClient.acctNum,   toClient.balance);

    snprintf(logMsg, sizeof(logMsg),
             "TRANSFER $%.2f from Acct#%d (%s %s) to Acct#%d (%s %s)",
             amount,
             fromClient.acctNum, fromClient.firstName, fromClient.lastName,
             toClient.acctNum,   toClient.firstName,   toClient.lastName);
    logTransaction(logMsg);
}

// ─── Menu ─────────────────────────────────────────────────────────────────────
unsigned int enterChoice(void)
{
    unsigned int menuChoice;

    printf("%s", "\n========== BANK ACCOUNT MENU ==========\n"
                 "1  - Export accounts to accounts.txt\n"
                 "2  - Update an account (charge/payment)  [PIN]\n"
                 "3  - Add a new account\n"
                 "4  - Delete an account                   [PIN]\n"
                 "--- View ---\n"
                 "5  - View a single account\n"
                 "6  - View all accounts\n"
                 "--- Transfer ---\n"
                 "7  - Transfer funds between accounts     [PIN]\n"
                 "--- Security ---\n"
                 "8  - Change PIN                          [PIN]\n"
                 "9  - Unlock account (admin)\n"
                 "10 - Exit\n"
                 "? ");

    scanf("%u", &menuChoice);
    return menuChoice;
}