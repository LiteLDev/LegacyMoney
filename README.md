# LegacyMoney

English | [简体中文](README.zh.md)  
Legacy LiteLoaderMoney for LeviLamina 

# Installation

## Use Lip

```bash
lip install github.com/LiteLDev/LegacyMoney
```

# Usage

| Command                     | Description                        | Permission |
|-----------------------------|------------------------------------|------------|
| /money query [player]       | Query your/other players's balance | Player/OP  |
| /money pay player amount    | Transfer money to a player         | Player     |
| /money set player amount    | Set player's balance               | OP         |
| /money add player amount    | Add player's balance               | OP         |
| /money reduce player amount | Reduce player's balance            | OP         |
| /money hist                 | Print your running account         | Player     |
| /money purge                | Clear your running account         | OP         |
| /money top                  | Balance ranking                    | Player     |
| /money_s                    | Command with TargetSelector        | Player     |

# Configuration File

```jsonc
{
    "language": "en",
    "def_money": 0, //Default money
    "pay_tax": 0.0 //Tax
}
```
