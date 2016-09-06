# CHAT 1.0
## REQUIRED LIBRARIES

[ZMQ - ZMQPP](https://github.com/Arq-Cliente-Servidor/hola-mundo/blob/master/README.md)

[SFML](http://www.sfml-dev.org/tutorials/2.0/start-linux.php)

## EXECUTING

Type in your terminal:

      make

    ./server

In other terminal:

      ./client <address> <login/register> <username> <password>

Solution to possible compiling error:

      export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/zmq/lib

## TYPES OF MESSAGES
## Register
Register in chat.

      register <username> <password>

**Example:**

      register pepito 123

## Login
Login in chat.

      login <username> <pasword>

**Example:**

      login pepito 123

## Send a message to a person

It is used to send a text message to a friend.  

      chatTo <friendName> <message>

**Example:**

      chatTo juanito hola amigo

## Add a friend    

It is used to add a friend, this is required to chat with other person.  

      addFriend <friendName>

**Example:**

      addFriend juanito
## Create a group

It is used to create a group in the chat.

      createGroup <groupName>

**Example:**

      createGroup utp

## Add a friend in a group

It is used to add a friend in a group, this is required to chat with a friend.

      addGroup <groupName> <friendName>

**Example:**

      addGroup utp juanito

## Send a message to a group

It is used to send a text message to a group.

      groupChat <groupName> <message>

**Example:**

      groupChat utp hola a todos

## Send a voice message to a person

It is used to send a voice message to a friend.

      recordTo <friendName>

## Send a voice message to a group

It is used to send a voice message to a group.

      recordGroup <groupName>


## Call a friend

It is used to call a friend.

      callTo <friendName>

## Call in group

It is used to call a group.

      callGroup <groupName>

## Stop a call with a friend

It is used to stop a call with a friend.

      stop

## Stop a call in a group

It is used to stop a call in a group.

      stopGroup

## List created groups

It is used to list the created groups.

      listGroup

## Leave a group

It is used to leave a group that you belong.

      leaveGroup

## Remove friend

It is used to remove a friend.

      removeFriend <friendName>

**Example**
      removeFriend juanito

## Logout
To close the chat.

      logout
