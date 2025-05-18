"use client"

import React, { useState, useEffect } from "react";
import { Textarea } from "@/components/ui/textarea"

interface JsonViewerProps {
    data: string | object;
    editable?: boolean;
    onChange?: (value: string) => void;
    height?: string;
    className?: string;
}

export function JsonViewer({
    data,
    editable = false,
    onChange,
    height = "auto",
    className = ""
}: JsonViewerProps) {
    const [jsonString, setJsonString] = useState("");
    const [isValid, setIsValid] = useState(true);

    useEffect(() => {
        try {
            if (typeof data === 'string') {
                setJsonString(data);
                JSON.parse(data);
                setIsValid(true);
            } else {
                const formatted = JSON.stringify(data, null, 2);
                setJsonString(formatted);
                setIsValid(true);
            }
        } catch (e) {
            if (typeof data === 'string') {
                setJsonString(data);
            } else {
                setJsonString(JSON.stringify({ error: "Invalid data" }));
            }
            setIsValid(false);
        }
    }, [data]);

    const handleChange = (value: string) => {
        setJsonString(value);
        try {
            JSON.parse(value);
            setIsValid(true);
            if (onChange) onChange(value);
        } catch (e) {
            setIsValid(false);
            // Still update the raw value even if invalid
            if (onChange) onChange(value);
        }
    };

    if (editable) {
        return (
            <div className={className}>
                <Textarea
                    className={`font-mono text-sm whitespace-pre-wrap ${isValid ? 'bg-slate-950 text-slate-50' : 'bg-red-50 text-red-500'} p-4 ${height !== 'auto' ? `h-${height}` : 'min-h-[150px]'}`}
                    value={jsonString}
                    onChange={(e) => handleChange(e.target.value)}
                />
                {!isValid && (
                    <div className="text-sm text-red-500 mt-1">
                        Invalid JSON format
                    </div>
                )}
            </div>
        );
    }

    // View-only mode
    try {
        const parsedData = typeof data === 'string' ? JSON.parse(data) : data;
        return (
            <div className={`rounded-md bg-slate-950 p-4 overflow-x-auto ${className}`}>
                <pre className="text-sm text-slate-50 whitespace-pre-wrap break-all">
                    {JSON.stringify(parsedData, null, 2)}
                </pre>
            </div>
        );
    } catch (e) {
        return (
            <div className="p-4 bg-red-50 text-red-500 rounded-md">
                Invalid JSON: {(e as Error).message}
            </div>
        );
    }
}
