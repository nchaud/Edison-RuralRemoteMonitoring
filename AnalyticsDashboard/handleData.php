<?php

    /*
    * Project created by Numaan Chaudhry 29/11/2016
    * Demo Instructable for #IntelIOT
    * This script handles incoming data from deployed devices
    */

    $deviceData = array();

    try {
        
        //Send acknowledgement to the device that server received the message
        echo "ACK";

        //Load common file in this try-catch
        require("common.php");
        
        //Enrich the data
        $deviceData["Source"]     = "WEB"; //Distinguish from SMS-based input source
        $deviceData["Data"]       = file_get_contents('php://input'); //Full RAW POST data
        $deviceData["ServerTime"] = date('Y-m-d H:i:s');
        $deviceData["RequestParams"]    = print_r($_REQUEST, TRUE);
        $deviceData["ServerParams"]     = print_r($_SERVER, TRUE);
        $deviceData["Headers"]    = print_r(getallheaders(), TRUE);
        
        $postData     = $deviceData["Data"];
        $postDataJson = json_decode($postData);
        $moduleId     = $postDataJson["ModuleID"];
        
        $documentToInsert = array("ModuleID" => $moduleId);
        $documentToInsert = array_merge($deviceData);
        
        $sensorCollection->insert($documentToInsert);
    }
    catch(Exception $e) {

        //Whatever happens, we do NOT want to lose this critical sensor data.
        //Store to a backup file
        
        file_put_contents("backup.dat", print_r($deviceData, true), FILE_APPEND);
        
        //Don't echo anything as the failure is on our side. Just log the error for analysis later.
        file_put_contents("log.txt", print_r($e, true), FILE_APPEND);
    }
?>