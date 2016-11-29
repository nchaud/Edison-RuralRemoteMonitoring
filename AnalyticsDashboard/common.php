<?php

    //This would normally be an environment variable like $_ENV_ from build server but hardcode for this instructable
    $environment  = "PROD";

    //Read the mongo connection string for this environment from a file
    $mongoCfgFile = "mongo." . $environment . ".config";
    $contents     = file_get_contents($mongoCfgFile);
    $jsonObj      = json_decode($contents);
    $connString   = $jsonObj->connectionString;

    //Initiate connection to MongoClient
    $m = new MongoClient($connString);

    $db = $m->solarData;

    $staticCollection = $db->staticData;
    $sensorCollection = $db->sensorData;

?>