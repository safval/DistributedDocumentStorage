# Distributed Document Storage

This is a document-oriented database mostly intended for use in CAD-like systems.
A document consist of objects. Each object has properties as key-value pairs.
Types of the objects and its properties defined by the library user.
Property can store link to another object to make graph relationship.
Transactions are the only way to make changes in the data base.
Transactions used for the undo-redo mechanism and remote documents syncronization.
It is possible to insert a document into another.
