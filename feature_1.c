#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// clientData structure definition
struct clientData
{
    unsigned int acctNum; // account number
    char lastName[15];    // account last name
    char firstName[10];   // account first name
    double balance;       // account balance
};

// prototypes (original)
unsigned int enterChoice(void);
void textFile(FILE *readPtr);
void updateRecord(FILE *fPtr);
void newRecord(FILE *fPtr);
void deleteRecord(FILE *fPtr);

// prototypes (new features)
void viewAccount(FILE *fPtr);
void viewAllAccounts(FILE *fPtr);
void transferFunds(FILE *fPtr);
void logTransaction(const char *description);

// ─── Logging helper ────────────────────────────────────────────────────────────
void logTransaction(const char *description)
{
    FILE *logPtr = fopen("transactions.log", "a");
    if (logPtr == NULL)
    {
        puts("Warning: Could not open transaction log.");
        return;
    }

    time_t now = time(NULL);
    char timeStr[26];
    strncpy(timeStr, ctime(&now), 25);
    timeStr[24] = '\0'; // remove newline

    fprintf(logPtr, "[%s] %s\n", timeStr, description);
    fclose(logPtr);
}

// ─── Main ──────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    FILE *cfPtr;
    unsigned int choice;

    if ((cfPtr = fopen("credit.dat", "rb+")) == NULL)
    {
        printf("%s: File could not be opened.\n", argv[0]);
        exit(-1);
    }

    while ((choice = enterChoice()) != 8)
    {
        switch (choice)
        {
        case 1:
            textFile(cfPtr);
            break;
        case 2:
            updateRecord(cfPtr);
            break;
        case 3:
            newRecord(cfPtr);
            break;
        case 4:
            deleteRecord(cfPtr);
            break;
        // ── NEW FEATURES ──
        case 5:
            viewAccount(cfPtr);
            break;
        case 6:
            viewAllAccounts(cfPtr);
            break;
        case 7:
            transferFunds(cfPtr);
            break;
        default:
            puts("Incorrect choice");
            break;
        }
    }

    fclose(cfPtr);
    puts("Goodbye!");
    return 0;
}

// ─── Original Functions ────────────────────────────────────────────────────────

void textFile(FILE *readPtr)
{
    FILE *writePtr;
    int result;
    struct clientData client = {0, "", "", 0.0};

    if ((writePtr = fopen("accounts.txt", "w")) == NULL)
    {
        puts("File could not be opened.");
    }
    else
    {
        rewind(readPtr);
        fprintf(writePtr, "%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");

        while (!feof(readPtr))
        {
            result = fread(&client, sizeof(struct clientData), 1, readPtr);
            if (result != 0 && client.acctNum != 0)
            {
                fprintf(writePtr, "%-6d%-16s%-11s%10.2f\n",
                        client.acctNum, client.lastName, client.firstName, client.balance);
            }
        }

        fclose(writePtr);
        puts("accounts.txt has been created.");
        logTransaction("Exported all accounts to accounts.txt");
    }
}

void updateRecord(FILE *fPtr)
{
    unsigned int account;
    double transaction;
    struct clientData client = {0, "", "", 0.0};
    char logMsg[128];

    printf("%s", "Enter account to update ( 1 - 100 ): ");
    scanf("%d", &account);

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account #%d has no information.\n", account);
    }
    else
    {
        printf("%-6d%-16s%-11s%10.2f\n\n",
               client.acctNum, client.lastName, client.firstName, client.balance);

        printf("%s", "Enter charge ( + ) or payment ( - ): ");
        scanf("%lf", &transaction);

        double oldBalance = client.balance;
        client.balance += transaction;

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
}

void deleteRecord(FILE *fPtr)
{
    struct clientData client;
    struct clientData blankClient = {0, "", "", 0};
    unsigned int accountNum;
    char logMsg[128];

    printf("%s", "Enter account number to delete ( 1 - 100 ): ");
    scanf("%d", &accountNum);

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account %d does not exist.\n", accountNum);
    }
    else
    {
        snprintf(logMsg, sizeof(logMsg),
                 "DELETE Acct#%d %s %s | Balance was %.2f",
                 client.acctNum, client.firstName, client.lastName, client.balance);

        fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
        fwrite(&blankClient, sizeof(struct clientData), 1, fPtr);

        printf("Account #%d deleted.\n", accountNum);
        logTransaction(logMsg);
    }
}

void newRecord(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    unsigned int accountNum;
    char logMsg[128];

    printf("%s", "Enter new account number ( 1 - 100 ): ");
    scanf("%d", &accountNum);

    fseek(fPtr, (accountNum - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum != 0)
    {
        printf("Account #%d already contains information.\n", client.acctNum);
    }
    else
    {
        printf("%s", "Enter lastname, firstname, balance\n? ");
        scanf("%14s%9s%lf", client.lastName, client.firstName, &client.balance);
        client.acctNum = accountNum;

        fseek(fPtr, (client.acctNum - 1) * sizeof(struct clientData), SEEK_SET);
        fwrite(&client, sizeof(struct clientData), 1, fPtr);

        printf("Account #%d created successfully.\n", accountNum);

        snprintf(logMsg, sizeof(logMsg),
                 "CREATE Acct#%d %s %s | Opening balance: %.2f",
                 client.acctNum, client.firstName, client.lastName, client.balance);
        logTransaction(logMsg);
    }
}

// ─── NEW FEATURE 1: View a Single Account ─────────────────────────────────────
void viewAccount(FILE *fPtr)
{
    unsigned int account;
    struct clientData client = {0, "", "", 0.0};

    printf("Enter account number to view ( 1 - 100 ): ");
    scanf("%d", &account);

    if (account < 1 || account > 100)
    {
        puts("Invalid account number.");
        return;
    }

    fseek(fPtr, (account - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account #%d does not exist.\n", account);
    }
    else
    {
        printf("\n%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
        printf("%-6d%-16s%-11s%10.2f\n",
               client.acctNum, client.lastName, client.firstName, client.balance);
    }
}

// ─── NEW FEATURE 2: View All Accounts on Screen ───────────────────────────────
void viewAllAccounts(FILE *fPtr)
{
    struct clientData client = {0, "", "", 0.0};
    int result;
    int count = 0;

    rewind(fPtr);
    printf("\n%-6s%-16s%-11s%10s\n", "Acct", "Last Name", "First Name", "Balance");
    printf("%-6s%-16s%-11s%10s\n", "----", "---------", "----------", "-------");

    while (!feof(fPtr))
    {
        result = fread(&client, sizeof(struct clientData), 1, fPtr);
        if (result != 0 && client.acctNum != 0)
        {
            printf("%-6d%-16s%-11s%10.2f\n",
                   client.acctNum, client.lastName, client.firstName, client.balance);
            count++;
        }
    }

    if (count == 0)
        puts("No accounts found.");
    else
        printf("\nTotal accounts: %d\n", count);
}

// ─── NEW FEATURE 3: Transfer Funds Between Accounts ───────────────────────────
void transferFunds(FILE *fPtr)
{
    unsigned int fromAcct, toAcct;
    double amount;
    struct clientData fromClient = {0, "", "", 0.0};
    struct clientData toClient   = {0, "", "", 0.0};
    char logMsg[256];

    printf("Enter source account number ( 1 - 100 ): ");
    scanf("%d", &fromAcct);
    printf("Enter destination account number ( 1 - 100 ): ");
    scanf("%d", &toAcct);

    if (fromAcct == toAcct)
    {
        puts("Source and destination accounts cannot be the same.");
        return;
    }
    if (fromAcct < 1 || fromAcct > 100 || toAcct < 1 || toAcct > 100)
    {
        puts("Invalid account number(s).");
        return;
    }

    // Read source
    fseek(fPtr, (fromAcct - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&fromClient, sizeof(struct clientData), 1, fPtr);

    // Read destination
    fseek(fPtr, (toAcct - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&toClient, sizeof(struct clientData), 1, fPtr);

    if (fromClient.acctNum == 0)
    {
        printf("Source account #%d does not exist.\n", fromAcct);
        return;
    }
    if (toClient.acctNum == 0)
    {
        printf("Destination account #%d does not exist.\n", toAcct);
        return;
    }

    printf("Transfer from: %-6d%-16s%-11s Balance: %10.2f\n",
           fromClient.acctNum, fromClient.lastName, fromClient.firstName, fromClient.balance);
    printf("Transfer to:   %-6d%-16s%-11s Balance: %10.2f\n",
           toClient.acctNum, toClient.lastName, toClient.firstName, toClient.balance);

    printf("Enter amount to transfer: ");
    scanf("%lf", &amount);

    if (amount <= 0)
    {
        puts("Transfer amount must be positive.");
        return;
    }
    if (fromClient.balance < amount)
    {
        printf("Insufficient funds. Available balance: %.2f\n", fromClient.balance);
        return;
    }

    // Perform transfer
    fromClient.balance -= amount;
    toClient.balance   += amount;

    // Write source back
    fseek(fPtr, (fromAcct - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&fromClient, sizeof(struct clientData), 1, fPtr);

    // Write destination back
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

// ─── Menu ──────────────────────────────────────────────────────────────────────
unsigned int enterChoice(void)
{
    unsigned int menuChoice;

    printf("%s", "\n========== BANK ACCOUNT MENU ==========\n"
                 "1 - Export accounts to accounts.txt\n"
                 "2 - Update an account (charge/payment)\n"
                 "3 - Add a new account\n"
                 "4 - Delete an account\n"
                 "--- New Features ---\n"
                 "5 - View a single account\n"
                 "6 - View all accounts\n"
                 "7 - Transfer funds between accounts\n"
                 "8 - Exit\n"
                 "? ");

    scanf("%u", &menuChoice);
    return menuChoice;
}