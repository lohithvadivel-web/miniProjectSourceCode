#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct clientData
{
    unsigned int acctNum;
    char lastName[15];
    char firstName[10];
    double balance;
};

// Function prototypes
void displayAll(FILE *fPtr);
void searchAccount(FILE *fPtr);
void deposit(FILE *fPtr);
void withdraw(FILE *fPtr);
unsigned int enterChoice(void);

int main()
{
    FILE *cfPtr;

    if ((cfPtr = fopen("credit.dat", "rb+")) == NULL)
    {
        printf("File could not be opened.\n");
        return 1;
    }

    int choice;

    while ((choice = enterChoice()) != 6)
    {
        switch (choice)
        {
        case 1:
            displayAll(cfPtr);
            break;
        case 2:
            searchAccount(cfPtr);
            break;
        case 3:
            deposit(cfPtr);
            break;
        case 4:
            withdraw(cfPtr);
            break;
        case 5:
            printf("Exiting...\n");
            break;
        default:
            printf("Invalid choice!\n");
        }
    }

    fclose(cfPtr);
    return 0;
}

// MENU
unsigned int enterChoice(void)
{
    int choice;
    printf("\n============================\n");
    printf("     BANK MANAGEMENT\n");
    printf("============================\n");
    printf("1. Display All Accounts\n");
    printf("2. Search Account\n");
    printf("3. Deposit Money\n");
    printf("4. Withdraw Money\n");
    printf("5. Exit\n");
    printf("6. End Program\n");
    printf("Enter choice: ");
    scanf("%d", &choice);

    return choice;
}

// DISPLAY ALL
void displayAll(FILE *fPtr)
{
    rewind(fPtr);
    struct clientData client;

    printf("\n%-6s %-15s %-10s %-10s\n", "Acct", "Last Name", "First Name", "Balance");

    while (fread(&client, sizeof(struct clientData), 1, fPtr))
    {
        if (client.acctNum != 0)
        {
            printf("%-6d %-15s %-10s %.2f\n",
                   client.acctNum, client.lastName,
                   client.firstName, client.balance);
        }
    }
}

// SEARCH ACCOUNT
void searchAccount(FILE *fPtr)
{
    int acc;
    struct clientData client;

    printf("Enter account number: ");
    scanf("%d", &acc);

    if (acc < 1 || acc > 100)
    {
        printf("Invalid account number!\n");
        return;
    }

    fseek(fPtr, (acc - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account not found.\n");
    }
    else
    {
        printf("\nAccount Found:\n");
        printf("%d %s %s %.2f\n",
               client.acctNum, client.lastName,
               client.firstName, client.balance);
    }
}

// DEPOSIT
void deposit(FILE *fPtr)
{
    int acc;
    double amount;
    struct clientData client;

    printf("Enter account number: ");
    scanf("%d", &acc);

    fseek(fPtr, (acc - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account not found.\n");
        return;
    }

    printf("Enter deposit amount: ");
    scanf("%lf", &amount);

    if (amount <= 0)
    {
        printf("Invalid amount!\n");
        return;
    }

    client.balance += amount;

    fseek(fPtr, (acc - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    printf("Deposit successful! New balance: %.2f\n", client.balance);
}

// WITHDRAW
void withdraw(FILE *fPtr)
{
    int acc;
    double amount;
    struct clientData client;

    printf("Enter account number: ");
    scanf("%d", &acc);

    fseek(fPtr, (acc - 1) * sizeof(struct clientData), SEEK_SET);
    fread(&client, sizeof(struct clientData), 1, fPtr);

    if (client.acctNum == 0)
    {
        printf("Account not found.\n");
        return;
    }

    printf("Enter withdraw amount: ");
    scanf("%lf", &amount);

    if (amount <= 0 || amount > client.balance)
    {
        printf("Invalid or insufficient balance!\n");
        return;
    }

    client.balance -= amount;

    fseek(fPtr, (acc - 1) * sizeof(struct clientData), SEEK_SET);
    fwrite(&client, sizeof(struct clientData), 1, fPtr);

    printf("Withdrawal successful! New balance: %.2f\n", client.balance);
}