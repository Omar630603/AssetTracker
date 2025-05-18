"use client"

import * as React from "react"
import { X, Check, ChevronsUpDown } from "lucide-react"
import { cn } from "@/lib/utils"
import { Button } from "@/components/ui/button"
import {
    Command,
    CommandEmpty,
    CommandGroup,
    CommandInput,
    CommandItem,
    CommandList,
} from "@/components/ui/command"
import {
    Popover,
    PopoverContent,
    PopoverTrigger,
} from "@/components/ui/popover"
import { Badge } from "@/components/ui/badge"

export interface MultiSelectItem {
    value: string
    label: string
}

interface MultiSelectProps {
    items: MultiSelectItem[]
    selectedValues: string[]
    onValueChange: (values: string[]) => void
    placeholder?: string
    emptyMessage?: string
    searchPlaceholder?: string
    className?: string
    disabled?: boolean
    maxDisplay?: number
}

export function MultiSelect({
    items,
    selectedValues = [],
    onValueChange,
    placeholder = "Select options...",
    emptyMessage = "No items found.",
    searchPlaceholder = "Search...",
    className,
    disabled = false,
    maxDisplay = 3,
}: MultiSelectProps) {
    const [open, setOpen] = React.useState(false)

    // Find the currently selected item labels to display
    const selectedItems = React.useMemo(() => {
        return items.filter((item) => selectedValues.includes(item.value))
    }, [items, selectedValues])

    const handleSelect = (value: string) => {
        if (selectedValues.includes(value)) {
            onValueChange(selectedValues.filter(v => v !== value))
        } else {
            onValueChange([...selectedValues, value])
        }
    }

    const handleRemove = (value: string) => {
        onValueChange(selectedValues.filter(v => v !== value))
    }

    return (
        <div className="w-full">
            <Popover open={open} onOpenChange={setOpen} modal={true}>
                <PopoverTrigger asChild>
                    <Button
                        variant="outline"
                        role="combobox"
                        aria-expanded={open}
                        disabled={disabled}
                        className={cn("w-full justify-between", className)}
                    >
                        <div className="flex flex-wrap gap-1 items-center">
                            {selectedItems.length > 0 ? (
                                <>
                                    {selectedItems.slice(0, maxDisplay).map((item) => (
                                        <Badge key={item.value} variant="secondary" className="mr-1">
                                            {item.label}
                                        </Badge>
                                    ))}
                                    {selectedItems.length > maxDisplay && (
                                        <Badge variant="secondary">+{selectedItems.length - maxDisplay}</Badge>
                                    )}
                                </>
                            ) : (
                                <span className="text-muted-foreground">{placeholder}</span>
                            )}
                        </div>
                        <ChevronsUpDown className="ml-2 h-4 w-4 shrink-0 opacity-50" />
                    </Button>
                </PopoverTrigger>
                <PopoverContent className="w-full p-0">
                    <Command>
                        <CommandInput placeholder={searchPlaceholder} className="h-9" />
                        <CommandList>
                            <CommandEmpty>{emptyMessage}</CommandEmpty>
                            <CommandGroup>
                                {items.map((item) => (
                                    <CommandItem
                                        key={item.value}
                                        onSelect={() => handleSelect(item.value)}
                                    >
                                        <div className="flex items-center">
                                            <Check
                                                className={cn(
                                                    "mr-2 h-4 w-4",
                                                    selectedValues.includes(item.value) ? "opacity-100" : "opacity-0"
                                                )}
                                            />
                                            {item.label}
                                        </div>
                                    </CommandItem>
                                ))}
                            </CommandGroup>
                        </CommandList>
                    </Command>
                </PopoverContent>
            </Popover>


            {selectedItems.length > 0 && (
                <Button
                    variant="ghost"
                    size="sm"
                    className="h-6 px-2 text-xs mt-2"
                    onClick={() => onValueChange([])}
                >
                    Clear all
                </Button>
            )}

        </div>
    )
}
